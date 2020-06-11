#include "test29.h"


class ProfileImpl : public Profile {
	std::unique_ptr<PrefService> prefs_;
public:
	ProfileImpl();
	~ProfileImpl() override;

	PrefService* GetPrefs() override;
	const PrefService* GetPrefs() const override;
};


ProfileImpl::ProfileImpl() : prefs_(new PrefService(1337)) {
	printf("ProfileImpl::ProfileImpl %p\n", this);
}


ProfileImpl::~ProfileImpl() {
	printf("ProfileImpl::~ProfileImpl %p\n", this);
}


PrefService* ProfileImpl::GetPrefs() {
	printf("ProfileImpl::GetPrefs @ %p\n", this);
	return const_cast<PrefService*>(static_cast<const ProfileImpl*>(this)->GetPrefs());
}

const PrefService* ProfileImpl::GetPrefs() const {
	printf("ProfileImpl::GetPrefs const @ %p\n", this);
	return prefs_.get();
}


Profile* getProfile() {
	return new ProfileImpl();
}
