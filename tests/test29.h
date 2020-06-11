#include <stdio.h>
#include <memory>


struct PrefService {
	int value;
	inline PrefService(int x) : value(x) {}
};


class Profile {
public:
	virtual ~Profile() {}

	virtual PrefService* GetPrefs() = 0;
	virtual const PrefService* GetPrefs() const = 0;
};


Profile* getProfile();
