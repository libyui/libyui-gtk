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
		{ "&product;", "foo" },
		{ "<p>&product;</p>", "<p>foo</p>" },
		{ "some text", "<body>some text</body>" },
		{ "<foo baa=Foo></foo>", "<foo baa=\"Foo\"></foo>" },
		{ "<br>", "<br/>" },
		{ "<hr>", "<hr/>" },
		// FIXME: we need to get <p> markers to work nicely.
		{ NULL, NULL }
	};
	for (int i = 0; aTests[i].in; i++) {
		gchar *out = ygutils_convert_to_xhmlt_and_subst (aTests[i].in, "foo");
		if (strcmp (out, aTests[i].out)) {
			fprintf (stderr, "Mis-converted entry %d XML '%s' vs '%s'\n",
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
