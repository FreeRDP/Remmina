/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2019 Antenore Gatta & Giovanni Panozzo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 * If you modify file(s) with this exception, you may extend this exception
 * to your version of the file(s), but you are not obligated to do so.
 * If you do not wish to do so, delete this exception statement from your
 * version.
 * If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *
 */


#include "kwallet_plugin.h"
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

int rp_kwallet_is_service_available(void)
{
	return wallet != 0;
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
    char *cpassword = (char *)malloc(pba.size()+1);
    strcpy(cpassword, pba.data());
    return cpassword;
}

void rp_kwallet_delete_password(const char *key)
{
    wallet->removeEntry(key);
}
