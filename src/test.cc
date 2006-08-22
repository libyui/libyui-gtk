//                       YaST2-GTK                                //
// YaST webpage - http://developer.novell.com/wiki/index.php/YaST //

#include <stdio.h>
#include <YUI.h>
#include <YGUtils.h>

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
		string mapped = YGUtils::mapKBAccel(aTests[i].in);
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

bool testFilterText ()
{
	fprintf (stderr, "Test filter text\t");
	struct {
		const char *in;
		const char *valid;
		const char *out;
	} aTests[] = {
		{ "Foo", "F", "F" },
		{ "Foo", "Fo", "Foo" },
		{ "Baa", "a", "aa" },
		{ "Kuckles", "Ks", "Ks" },
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		string filtered = YGUtils::filterText
			(aTests[i].in, strlen (aTests[i].in), aTests[i].valid);
		if (filtered != aTests[i].out) {
			fprintf (stderr, "Mis-filtered text '%s' vs '%s'\n",
				 filtered.c_str(), aTests[i].out);
			return false;
		}
		fprintf (stderr, "%d ", i);
	}
	fprintf (stderr, "\n");
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
		{ "<p>foo</p>", "<p>foo</p>" },
		// product substitution
		{ "&product;", "<body>foo</body>" },
		{ " <p>&product;</p>", "<p>foo</p>" },
		// outer tag
		{ "some text", "<body>some text</body>" },
		// unquoted attributes
		{ "<foo baa=Foo></foo>", "<foo baa=\"Foo\"></foo>" },
		// break tags
		{ "<br>", "<br/>" },
		{ "<hr>", "<hr/>" },
		// unclosed tags
		{ "<p>", "<p></p>" },
		{ "<b>unclosed", "<b>unclosed</b>" },
		{ "<b><i>bold</i>", "<b><i>bold</i></b>" },
		{ "<i><i>bold</i>", "<i><i>bold</i></i>" },
		{ "<unclosed foo=baa>",
		  "<unclosed foo=\"baa\"></unclosed>" },
		// unclosed 'early close' tags
		{ "<i>Some<p>text<p> here<p></i>",
		  "<i>Some<p>text</p><p> here</p><p></p></i>" },
		{ "<ul><li>foo<li>baa",
		  "<ul><li>foo</li><li>baa</li></ul>" },
		{ "no outer<p>unclosed<p>several<b>unclosed bold",
		  "<body>no outer<p>unclosed</p><p>several<b>unclosed bold</b></p></body>" },
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		gchar *out = ygutils_convert_to_xhmlt_and_subst (aTests[i].in, "foo");
		if (strcmp (out, aTests[i].out)) {
			fprintf (stderr, "Mis-converted entry %d XML '%s' should be '%s'\n",
				 i, out, aTests[i].out);
			return false;
		}
		fprintf (stderr, "%d ", i);
		g_free (out);
	}
	fprintf (stderr, "\n");
	return true;
}

int main (int argc, char **argv)
{
	bool bSuccess = true;

	bSuccess &= testMapKBAccel();
	bSuccess &= testFilterText();
	bSuccess &= testXHtmlConvert();

	return !bSuccess;
}
