
#include "kwallet_plugin.h"
#include <stdio.h>
#include <KWallet>

static KWallet::Wallet* wallet;

static char folderName[] = "Remmina";

int rp_kwallet_init(void)
{
	QString s = KWallet::Wallet::LocalWallet();
	wallet = KWallet::Wallet::openWallet(s, 0);
	if (!wallet) {
		return 0;
	}

	if (!wallet->createFolder(folderName)) {
		delete wallet;
		return 0;
	}
	if (!wallet->setFolder(folderName)) {
		delete wallet;
		return 0;
	}

	return 1;
}

void rp_kwallet_store_password(const char *key, const char *password)
{
    wallet->writePassword(key, password);
}

char *rp_kwallet_get_password(const char *key)
{
    QString password;
    if (wallet->readPassword(key, password) != 0) {
        return NULL;
    }
    QByteArray pba = password.toUtf8();
    char *cpassword = new char[pba.size()+1];
    strcpy(cpassword, pba.data());
    return cpassword;
}

void rp_kwallet_delete_password(const char *key)
{
    wallet->removeEntry(key);
}



