/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/
/* YGtkPkgSearchEntry, zypp query string attributes */
// check the header file for information about this widget

/*
  Textdomain "gtk"
 */

#include "YGi18n.h"
#include "config.h"
#include "YGUtils.h"
#include "ygtkpkgsearchentry.h"
#include "YGPackageSelector.h"
#include <gtk/gtk.h>

struct YGtkPkgSearchEntry::Impl {
	GtkWidget *box, *entry, *combo;
};

static void entry_icons_sync (GtkWidget *widget)
{
	static GdkColor yellow = { 0, 0xf7f7, 0xf7f7, 0xbdbd };
	static GdkColor black = { 0, 0, 0, 0 };

	GtkEntry *entry = GTK_ENTRY (widget);  // show clear icon if text
	const gchar *name = gtk_entry_get_text (entry);
	bool showIcon = *name;
	if (showIcon != gtk_entry_get_icon_activatable (entry, GTK_ENTRY_ICON_SECONDARY)) {
		gtk_entry_set_icon_activatable (entry,
			GTK_ENTRY_ICON_SECONDARY, showIcon);
		gtk_entry_set_icon_from_stock (entry,
			GTK_ENTRY_ICON_SECONDARY, showIcon ? GTK_STOCK_CLEAR : NULL);

		if (showIcon) {
			gtk_entry_set_icon_tooltip_text (entry,
				GTK_ENTRY_ICON_SECONDARY, _("Clear"));
			gtk_widget_modify_base (widget, GTK_STATE_NORMAL, &yellow);
			gtk_widget_modify_text (widget, GTK_STATE_NORMAL, &black);
		}
		else {  // revert
			gtk_widget_modify_base (widget, GTK_STATE_NORMAL, NULL);
			gtk_widget_modify_text (widget, GTK_STATE_NORMAL, NULL);
		}
	}
}

static void entry_changed_cb (GtkEditable *editable, YGtkPkgSearchEntry *pThis)
{
	int item = gtk_combo_box_get_active (GTK_COMBO_BOX (pThis->impl->combo));
	pThis->notifyDelay (item == 0 ? 150 : 500);
	entry_icons_sync (GTK_WIDGET (editable));
}

static void combo_changed_cb (GtkComboBox *combo, YGtkPkgSearchEntry *pThis)
{
	const gchar *name = gtk_entry_get_text (GTK_ENTRY (pThis->impl->entry));
	if (*name)  // unless entry has text, no need to refresh query
		pThis->notify();
	gtk_editable_select_region (GTK_EDITABLE (pThis->impl->entry), 0, -1);
	gtk_widget_grab_focus (pThis->impl->entry);
}

static void icon_press_cb (GtkEntry *entry, GtkEntryIconPosition pos,
                            GdkEvent *event, YGtkPkgSearchEntry *pThis)
{
	if (pos == GTK_ENTRY_ICON_PRIMARY)
		gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
	else
		gtk_entry_set_text (entry, "");
	gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static void activate_cb (GtkEntry *entry, GtkWidget *widget)
{ gtk_widget_grab_focus (widget); }

YGtkPkgSearchEntry::YGtkPkgSearchEntry()
: YGtkPkgQueryWidget(), impl (new Impl())
{
	impl->entry = gtk_entry_new();
	gtk_widget_set_size_request (impl->entry, 180, -1);
	g_signal_connect (G_OBJECT (impl->entry), "realize",  // grab focus at start
	                  G_CALLBACK (gtk_widget_grab_focus), NULL);
	g_signal_connect (G_OBJECT (impl->entry), "changed",
	                  G_CALLBACK (entry_changed_cb), this);
	g_signal_connect (G_OBJECT (impl->entry), "icon-press",
	                  G_CALLBACK (icon_press_cb), this);
	gtk_widget_set_tooltip_markup (impl->entry,
		_("<b>Package search:</b> Use spaces to separate your keywords.\n"
		"(usage example: a name search for \"yast dhcp\" would match yast dhcpd tool)"));

	gtk_entry_set_icon_from_stock (GTK_ENTRY (impl->entry),
		GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
	gtk_entry_set_icon_activatable (GTK_ENTRY (impl->entry), GTK_ENTRY_ICON_PRIMARY, TRUE);

	std::string label_str (YGUtils::mapKBAccel (_("&Find:")));
	GtkWidget *label = gtk_label_new_with_mnemonic (label_str.c_str());
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), impl->entry);

	impl->combo = gtk_combo_box_text_new();
	GtkComboBoxText *combot = GTK_COMBO_BOX_TEXT (impl->combo);
	gtk_combo_box_text_append (combot, 0, _("Name & Summary"));
	gtk_combo_box_text_append (combot, 0, _("Description"));
	if (!YGPackageSelector::get()->onlineUpdateMode()) {
		gtk_combo_box_text_append (combot, 0, _("File name"));
		gtk_combo_box_text_append (combot, 0, "RPM Provides");
		gtk_combo_box_text_append (combot, 0, "RPM Requires");
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (impl->combo), 0);
	YGUtils::shrinkWidget (impl->combo);
	g_signal_connect (G_OBJECT (impl->combo), "changed",
	                  G_CALLBACK (combo_changed_cb), this);
	gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (impl->combo),
		YGUtils::empty_row_is_separator_cb, GINT_TO_POINTER (0), NULL);

	impl->box = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (impl->box), label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (impl->box), impl->entry, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (impl->box), gtk_label_new (_("by")), FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (impl->box), impl->combo, TRUE, TRUE, 0);
	gtk_widget_show_all (impl->box);
}

YGtkPkgSearchEntry::~YGtkPkgSearchEntry()
{ delete impl; }

GtkWidget *YGtkPkgSearchEntry::getWidget()
{ return impl->box; }

void YGtkPkgSearchEntry::clearSelection()
{ setText (Ypp::PoolQuery::NAME, ""); }

bool YGtkPkgSearchEntry::writeQuery (Ypp::PoolQuery &query)
{
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (impl->entry));
	if (*text) {
		int item = gtk_combo_box_get_active (GTK_COMBO_BOX (impl->combo));
		if (item >= 0 && item <= 1) {
			int attrbs;
			switch (item) {
				case 0:  // name & summary
					attrbs = Ypp::StrMatch::NAME | Ypp::StrMatch::SUMMARY;
					break;
				case 1:  // description
					attrbs = Ypp::StrMatch::DESCRIPTION;
					break;
			}

			Ypp::StrMatch *match = new Ypp::StrMatch (attrbs);
			const gchar delimiter[2] = { ' ', '\0' };
			gchar **keywords = g_strsplit (text, delimiter, -1);
			for (gchar **i = keywords; *i; i++)
				match->add (*i);
			g_strfreev (keywords);

			query.addCriteria (match);
		}
		else {
			query.setStringMode (false, Ypp::PoolQuery::CONTAINS);

			switch (item) {
				case 2:  // filelist
					query.addStringAttribute (Ypp::PoolQuery::PROVIDES);
					query.addStringAttribute (Ypp::PoolQuery::FILELIST);
					break;
				case 3:  // provides
					query.addStringAttribute (Ypp::PoolQuery::PROVIDES);
					break;
				case 4:  // requires
					query.addStringAttribute (Ypp::PoolQuery::REQUIRES);
					break;
			}

			query.addStringOr (text);
		}
		return true;
	}
	return false;
}

static gboolean patterns_link_cb (GtkLabel *label, gchar *uri, YGtkPkgSearchEntry *pThis)
{ YGPackageSelector::get()->showFilterWidget (uri); return TRUE; }

static bool any_pattern_contains (const char *name)
{
	if (strstr (name, " ")) return false;

	// roll on some criteria to find whole-word match (the zypp one doesnt cut it)
	struct WholeWordMatch : public Ypp::Match {
		WholeWordMatch (const char *word)
		: word (word), len (strlen (word)) {}

		virtual bool match (Ypp::Selectable &sel) {
			std::string name (sel.name());
			const char *i = name.c_str();
			while ((i = strcasestr (i, word))) {
				bool starts = (i == word) || (i[-1] != ' ');
				bool ends = (i[len+1] == '\0') || (i[len+1] != ' ');
				if (starts && ends)
					return true;
				i += len;
			}
			return false;
		}
		private: const char *word; int len;
	};

	Ypp::PoolQuery query (Ypp::Selectable::PATTERN);
	query.addCriteria (new WholeWordMatch (name));
	//query.addStringOr (name);
	return query.hasNext();
}

GtkWidget *YGtkPkgSearchEntry::createToolbox()
{
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (impl->entry));
	int item = gtk_combo_box_get_active (GTK_COMBO_BOX (impl->combo));
	if (*text)
		switch (item) {
			case 0:
				if (any_pattern_contains (text)) {
					char *_text = g_strdup_printf (_("%sPatterns are available%s"
						" that correspond to your search criteria."),
						"<a href=\"patterns\">", "</a>");
					GtkWidget *label = gtk_label_new (_text);
					g_free (_text);
					gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
					gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
					g_signal_connect (G_OBJECT (label), "activate-link",
					                  G_CALLBACK (patterns_link_cb), this);
					GtkWidget *icon = gtk_image_new_from_stock (
						GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_BUTTON);
					GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
					gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
					gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
					gtk_widget_show_all (hbox);
					return hbox;
				}
				break;
			case 2: {
				GtkWidget *label = gtk_label_new (
					_("Search by file name only reliable for installed packages."));
				gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
				GtkWidget *icon = gtk_image_new_from_stock (
					GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
				GtkWidget *hbox = gtk_hbox_new (FALSE, 6);
				gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
				gtk_widget_show_all (hbox);
				return hbox;
			}
		}
	return NULL;
}

void YGtkPkgSearchEntry::setText (Ypp::PoolQuery::StringAttribute attrb, const std::string &text)
{
	int index = 0;
	switch (attrb) {
		case Ypp::PoolQuery::NAME: case Ypp::PoolQuery::SUMMARY: break;
		case Ypp::PoolQuery::DESCRIPTION: index = 1; break;
		case Ypp::PoolQuery::FILELIST: index = 2; break;
		case Ypp::PoolQuery::PROVIDES: index = 3; break;
		case Ypp::PoolQuery::REQUIRES: index = 4; break;
	}

	g_signal_handlers_block_by_func (impl->entry, (gpointer) entry_changed_cb, this);
	g_signal_handlers_block_by_func (impl->combo, (gpointer) combo_changed_cb, this);
	gtk_combo_box_set_active (GTK_COMBO_BOX (impl->combo), index);
	gtk_entry_set_text (GTK_ENTRY (impl->entry), text.c_str());
	entry_icons_sync (impl->entry);
	g_signal_handlers_unblock_by_func (impl->entry, (gpointer) entry_changed_cb, this);
	g_signal_handlers_unblock_by_func (impl->combo, (gpointer) combo_changed_cb, this);
}

void YGtkPkgSearchEntry::setActivateWidget (GtkWidget *widget)
{
	g_signal_connect (G_OBJECT (impl->entry), "activate",
	                  G_CALLBACK (activate_cb), widget);
}

Ypp::PoolQuery::StringAttribute YGtkPkgSearchEntry::getAttribute()
{
	int item = gtk_combo_box_get_active (GTK_COMBO_BOX (impl->combo));
	switch (item) {
		case 0: return Ypp::PoolQuery::NAME; break;
		case 1: return Ypp::PoolQuery::DESCRIPTION; break;
		case 2: return Ypp::PoolQuery::FILELIST; break;
		case 3: return Ypp::PoolQuery::PROVIDES; break;
		case 4: return Ypp::PoolQuery::REQUIRES; break;
	}
	return (Ypp::PoolQuery::StringAttribute) 0;
}

std::list <std::string> YGtkPkgSearchEntry::getText()
{
	std::list <std::string> keywords;
	const char *text = gtk_entry_get_text (GTK_ENTRY (impl->entry));
	const gchar delimiter[2] = { ' ', '\0' };
	gchar **_keywords = g_strsplit (text, delimiter, -1);
	for (gchar **i = _keywords; *i; i++)
		if (**i)
			keywords.push_back (*i);
	g_strfreev (_keywords);
	return keywords;
}

std::string YGtkPkgSearchEntry::getTextStr()
{ return gtk_entry_get_text (GTK_ENTRY (impl->entry)); }

