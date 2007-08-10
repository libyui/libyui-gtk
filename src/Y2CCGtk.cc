/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#include <config.h>
#include <YGUI.h>
#include <YUIComponent.h>
#include <Y2CCUI.h>
#include <ycp/y2log.h>

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
	Y2CCGtk () : Y2CCUI() { };

	bool isServerCreator () const { return true; };
	
	Y2Component *create (const char * name) const
	{
		y2milestone( "Creating %s component", name );
		if (!strcmp (name, "gtk") ) {
			Y2Component* ret = YUIComponent::uiComponent ();
			if (!ret || ret->name () != name) {
				y2debug ("UI component is %s, creating %s", ret? ret->name().c_str() : "NULL", name);
				ret = new YGUIComponent();
			}
			return ret;
		}
		else
			return 0;
	}
};

// Singleton plugin registration instance.
Y2CCGtk g_y2ccgtk;


