/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <YUI.h>
#include <gtk/gtk.h>
#include "YGUtils.h"

bool testMapKBAccel()
{
	fprintf (stderr, "Test map KB accels\t");
	struct {
		const char *in;
		const char *out;
	} aTests[] = {
		{ "Foo&", "Foo_" },
		{ "B&aa", "B_aa" },
		{ "&Foo", "_Foo" },
		{ "_Foo", "__Foo" },
// FIXME - unfortunately it seems yast has noescaping policy
// for labels (hmm) at least judging from the code at
// ncurses/src/NCstring.cc (getHotKey)
//		{ "&&Foo", "&Foo" },
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		std::string mapped = YGUtils::mapKBAccel(aTests[i].in);
		if (mapped != aTests[i].out) {
			fprintf (stderr, "Mis-mapped accel '%s' vs '%s'\n",
				 mapped.c_str(), aTests[i].out);
			return false;
		}
		fprintf (stderr, "%d ", i);
	}
	fprintf (stderr, "\n");
	return true;
}

#include "ygtkrichtext.h"

bool testParse(const char *xml)
{
  GError *error = NULL;
  GMarkupParser parser = { 0, };
  GMarkupParseContext *ctx = g_markup_parse_context_new (&parser, GMarkupParseFlags (0), NULL, NULL);

  if (!g_markup_parse_context_parse (ctx, xml, -1, &error))
    {
      fprintf (stderr, "Invalid XML: '%s'\n '%s'\n", xml, error->message);
      return false;
    }
  g_markup_parse_context_free (ctx);

  return true;
}

bool testXHtmlConvert()
{
	fprintf (stderr, "Test HTML->XML rewrite \t");
	struct {
		const char *in;
		const char *out;
	} aTests[] = {
		// preservation
		{ "<p>foo</p>", "<body><p>foo</p></body>" },
		// outer tag
		{ "some text", "<body>some text</body>" },
		// unquoted attributes
		{ "<foo baa=Baz></foo>", "<body><foo baa=\"Baz\"></foo></body>" },
		// break tags
		{ "<br>", "<body><br/></body>" },
		{ "<hr>", "<body><hr/></body>" },
		// close with different case ...
		{ "<p>Foo</P>", "<body><p>Foo</p></body>" },
		// unclosed tags
		{ "<p>", "<body><p></p></body>" },
		{ "<b>unclosed", "<body><b>unclosed</b></body>" },
		{ "<b><i>bold</i>", "<body><b><i>bold</i></b></body>" },
		{ "<i><i>bold</i>", "<body><i><i>bold</i></i></body>" },
		{ "<unclosed foo=baa>",
		  "<body><unclosed foo=\"baa\"></unclosed></body>" },
		// unclosed 'early close' tags
		{ "<i>Some<p>text<p> here<p></i>",
		  "<body><i>Some<p>text</p><p> here</p><p></p></i></body>" },
		{ "<ul><li>foo<li>baa",
		  "<body><ul><li>foo</li><li>baa</li></ul></body>" },
		{ "no outer<p>unclosed<p>several<b>unclosed bold",
		  "<body>no outer<p>unclosed</p><p>several<b>unclosed bold</b></p></body>" },
		// comment
		{ "we need <b>to <!-- really need to? --> do something</b> about it.",
		  "<body>we need <b>to do something</b> about it.</body>" },
		{ "&amp;", "<body>&amp;</body>" },
		{ "&amp", "<body>&amp;amp</body>" },
		{ "&amp; foo", "<body>&amp; foo</body>" },
		{ "&amp foo", "<body>&amp;amp foo</body>" },
		{ "<pre>https://foo.com/regsvc-1.0/?lang=de-DE&guid=1529f49dc701449fbd854aebf7e40806&command=interactive</pre>\n",
		  "<body><pre>https://foo.com/regsvc-1.0/?lang=de-DE&amp;guid=1529f49dc701449fbd854aebf7e40806&amp;command=interactive</pre> </body>" },
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		gchar *out = ygutils_convert_to_xhtml (aTests[i].in);
		if (strcmp (out, aTests[i].out)) {
			fprintf (stderr, "Mis-converted entry %d XML '%s' should be '%s'\n",
				 i, out, aTests[i].out);
			return false;
		}
		if (!testParse (out))
		  return false;

		fprintf (stderr, "%d ", i);
		g_free (out);
	}
	fprintf (stderr, "\n");
	return true;
}

bool testMarkupEscape()
{
	fprintf (stderr, "Test markup escape\t");
	struct {
		const char *in;
		const char *out;
		bool escape_br;
	} aTests[] = {
		{ "< text />", "&lt; text /&gt;" },
		{ "um\ndois\ntres", "um\ndois\ntres" },
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		std::string out (YGUtils::escapeMarkup (aTests[i].in));
		if (out != aTests[i].out) {
			fprintf (stderr, "Mis-converted entry %d XML '%s' should be '%s'\n",
				 i, out.c_str(), aTests[i].out);
			return false;
		}
		fprintf (stderr, "%d ", i);
	}
	fprintf (stderr, "\n");
	return true;
}

bool testTruncate()
{
	fprintf (stderr, "Test truncate\t");
	struct {
		const char *in;
		const char *out;
		int length, pos;
	} aTests[] = {
		{ "this-is-a-very-long-and-tedious-string", "this-is-a-very-lo...", 20, 1 },
		{ "this-is-a-very-long-and-tedious-string", "...nd-tedious-string", 20, -1 },
		{ "this-is-a-very-long-and-tedious-string", "this-is-a...s-string", 20, 0 },
		{ "this-is-a-very-long-and-tedious-string2", "this-is-...s-string2", 20, 0 },
		{ "abc", "abc", 3, 1 },
		{ "abcd", "...", 3, 1 },
		{ "abcd", "...", 3, -1 },
		{ "abcd", "...", 3, 0 },
		{ "abcdef", "a...", 4, 0 },
		{ "áéíóúçö", "áé...", 5, 1 },
		{ "áéíóúçö", "áéí...", 6, 1 },
		{ "áéíóúçö", "...çö", 5, -1 },
		{ "áéíóúçö", "...úçö", 6, -1 },
		{ "áéíóúçö", "á...ö", 5, 0 },
		{ "áéíóúçö", "á...çö", 6, 0 },
		{ "áéíóú", "áéíóú", 5, 0 },
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		std::string out = YGUtils::truncate (aTests[i].in, aTests[i].length, aTests[i].pos);
		if (out != aTests[i].out) {
			fprintf (stderr, "Mis-converted entry %d truncate '%s' should be '%s'\n",
				 i, out.c_str(), aTests[i].out);
			return false;
		}
		fprintf (stderr, "%d ", i);
	}
	fprintf (stderr, "\n");
	return true;
}

bool testHeaderize()
{
	fprintf (stderr, "Test headerize\t");
	struct {
		const char *in;
		const char *out;
		gboolean    cut;
	} aTests[] = {
		{ "test", "test", FALSE },
		{ "<h1>Purpose</h1><p>This tool lets you install, remove, and update applications.</p><p>openSUSE's software management",
		  "This tool lets you install, remove, and update applications.", TRUE },
		{ "\n<p><big><b>Section List</b></big><br>\n<P>From <B>Other</B>,\nyou can manually edit the boot loader configuration files, clear the current\n"
		  "configuration and propose a new configuration, start from scratch, or reread\nthe configuration saved on your disk. If you have multiple Linux systems installed,",
		  "From Other, you can manually edit the boot loader configuration files, clear the current configuration and "
		  "propose a new configuration, start from scratch, or reread the configuration saved on your disk.", TRUE },
		{ "<p><big><b>Sound Cards</b><big></p><P>Select an unconfigured card from the list and press <B>Edit</B> to\nconfigure it. If the card was not detected, press <B>Add</B> and\nconfigure the card manually.</P>",
		  "Select an unconfigured card from the list and press Edit to configure it.", TRUE },
		{ "\n<p><b><big>Service Start</big></b><br>\nTo start the service every time your computer is booted, set\n"
		  "<b>Enable firewall</b>. Otherwise set <b>Disable firewall</b>.</p>\n<p><b><big>Switch On or Off</big></b><br>",
		  "To start the service every time your computer is booted, set Enable firewall.", TRUE },
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		gboolean cut = FALSE;
		char *out = ygutils_headerize_help (aTests[i].in, &cut);
		if (strcmp (aTests[i].out, out)) {
			fprintf (stderr, "Mis-headerized test %d:\n-- \n%s\n-- should be --\n%s\n-- xhtml --\n%s\n-- \n",
				 i, out, aTests[i].out, ygutils_convert_to_xhtml (aTests[i].in));
			return false;
		}
		if (!!aTests[i].cut != !!cut)
			fprintf (stderr, "Mis-labelled as cut (%d)\n", !!cut);
		fprintf (stderr, "%d ", i);
	}
	fprintf (stderr, "\n");
	return true;
}

int main (int argc, char **argv)
{
	bool bSuccess = true;

	bSuccess &= testMapKBAccel();
	bSuccess &= testXHtmlConvert();
	bSuccess &= testMarkupEscape();
	bSuccess &= testTruncate();
	bSuccess &= testHeaderize();

	return !bSuccess;
}

