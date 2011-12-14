/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _TelepathyQt_pending_contacts_internal_h_HEADER_GUARD_
#define _TelepathyQt_pending_contacts_internal_h_HEADER_GUARD_

#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/Types>

namespace Tp
{

class TP_QT_NO_EXPORT PendingAddressingGetContacts : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingAddressingGetContacts)

public:
    PendingAddressingGetContacts(const ConnectionPtr &connection,
            const QString &vcardField, const QStringList &vcardAddresses);
    PendingAddressingGetContacts(const ConnectionPtr &connection,
            const QStringList &uris);
    ~PendingAddressingGetContacts();

    UIntList validHandles() const { return mValidHandles; }

    bool isForVCardAddresses() const { return !mVCardField.isEmpty(); }
    QString vcardField() const { return mVCardField; }
    QStringList vcardAddresses() const { return mVCardAddresses; }

    bool isForUris() const { return !mUris.isEmpty(); }
    QStringList uris() const { return mUris; }

    QStringList validAddresses() const { return mValidAddresses; }
    QStringList invalidAddresses() const { return mInvalidAddresses; }

private Q_SLOTS:
    void onGetContactsByVCardFieldFinished(QDBusPendingCallWatcher* watcher);
    void onGetContactsByURIFinished(QDBusPendingCallWatcher* watcher);

private:
    ConnectionPtr mConnection;

    UIntList mValidHandles;

    QString mVCardField;
    QStringList mVCardAddresses;
    QStringList mUris;

    QStringList mValidAddresses;
    QStringList mInvalidAddresses;
};

} // Tp

#endif
