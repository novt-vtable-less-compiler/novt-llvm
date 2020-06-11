#include "test29.h"

int main() {
	Profile* profile = getProfile();
	const Profile* profile2 = profile;

	int value = profile2->GetPrefs()->value;
	printf("Value = %d\n", value);

	value = profile->GetPrefs()->value;
	printf("Value = %d\n", value);

	delete profile;

	return 0;
}
