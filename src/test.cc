#include <stdio.h>
#include <YUI.h>
#include <YGUtils.h>

bool testMapKBAccel()
{
	fprintf (stderr, "Test map KB accels\n");
	const char *aTests[] =
		{ "Foo&", "Foo_",
		  "B&aa", "B_aa",
		  "&Foo", "_Foo",
		  "_Foo", "__Foo",
// FIXME - unfortunately it seems yast has noescaping policy
// for labels (hmm) at least judging from the code at
// ncurses/src/NCstring.cc (getHotKey)
//		  "&&Foo", "&Foo",
		  NULL };
	for (int i = 0; aTests[i]; i += 2) {
		string mapped = YGUtils::mapKBAccel(aTests[i]);
		if (mapped != aTests[i+1]) {
			fprintf (stderr, "Mis-mapped accel '%s' vs '%s'\n",
				 mapped.c_str(), aTests[i+1]);
			return false;
		}
	}
	return true;
}

int main (int argc, char **argv)
{
	bool bSuccess = testMapKBAccel();
	return !bSuccess;
}
