/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include <YUIComponent.h>
#include <ycp/y2log.h>

#if YAST2_VERSION >= 2015006
// New UI plugin work
#  include <Y2CCUI.h>
#  define NEW_UI_PLUGIN
#  define Y2CCUI_INIT
#else
#  define Y2CCUI      Y2ComponentCreator
#  define Y2CCUI_INIT Y2ComponentBroker::BUILTIN
#endif

class YGUIComponent : public YUIComponent
{
public:

	YGUIComponent() : YUIComponent() {}

	virtual string name() const { return "gtk"; }

	virtual YUI *createUI (int argc, char **argv,
			       bool with_threads, const char * macro_file)
	{
#ifdef IMPL_DEBUG
		fprintf (stderr, "Create a gtk+ UI: %d '%s' !\n", with_threads, macro_file);
#endif
		return new YGUI (argc, argv, with_threads, macro_file);
	}
};

class Y2CCGtk : public Y2CCUI
{
public:
	Y2CCGtk () : Y2CCUI (Y2CCUI_INIT) { };

	bool isServerCreator () const { return true; };
	
	Y2Component *create (const char * name) const
	{
		y2milestone( "Creating %s component", name );
		if (!strcmp (name, "gtk") ) {
#ifdef NEW_UI_PLUGIN
			Y2Component* ret = YUIComponent::uiComponent ();
			if (!ret || ret->name () != name) {
				y2debug ("UI component is %s, creating %s", ret? ret->name().c_str() : "NULL", name);
				ret = new YGUIComponent();
			}
			return ret;
#else
            return new YGUIComponent();
#endif
		}
		else
			return 0;
	}
};

// Singleton plugin registration instance.
Y2CCGtk g_y2ccgtk;


