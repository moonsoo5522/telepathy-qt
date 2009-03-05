/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008 Nokia Corporation
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

#include <TelepathyQt4/Client/AccountManager>

#include "TelepathyQt4/Client/_gen/account-manager.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-manager.moc.hpp"
#include "TelepathyQt4/_gen/cli-account-manager-body.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Client/Account>
#include <TelepathyQt4/Client/PendingAccount>
#include <TelepathyQt4/Client/PendingReady>
#include <TelepathyQt4/Constants>

#include <QQueue>
#include <QSet>
#include <QTimer>

/**
 * \addtogroup clientsideproxies Client-side proxies
 *
 * Proxy objects representing remote service objects accessed via D-Bus.
 *
 * In addition to providing direct access to methods, signals and properties
 * exported by the remote objects, some of these proxies offer features like
 * automatic inspection of remote object capabilities, property tracking,
 * backwards compatibility helpers for older services and other utilities.
 */

/**
 * \defgroup clientaccount Account and Account Manager proxies
 * \ingroup clientsideproxies
 *
 * Proxy objects representing the Telepathy Account Manager and the Accounts
 * that it manages, and their optional interfaces.
 */

namespace Telepathy
{
namespace Client
{

struct AccountManager::Private
{
    Private(AccountManager *parent);
    ~Private();

    void init();

    static void introspectMain(Private *self);

    void setAccountPaths(QSet<QString> &set, const QVariant &variant);

    // Public object
    AccountManager *parent;

    // Instance of generated interface class
    AccountManagerInterface *baseInterface;

    ReadinessHelper *readinessHelper;

    // Introspection
    QStringList interfaces;
    QSet<QString> validAccountPaths;
    QSet<QString> invalidAccountPaths;
    QMap<QString, QSharedPointer<Account> > accounts;
};

AccountManager::Private::Private(AccountManager *parent)
    : parent(parent),
      baseInterface(new AccountManagerInterface(parent->dbusConnection(),
                    parent->busName(), parent->objectPath(), parent))
{
    debug() << "Creating new AccountManager:" << parent->busName();

    ReadinessHelper::Introspectables introspectables;

    // As AccountManager does not have predefined statuses let's simulate one (0)
    ReadinessHelper::Introspectable introspectableCore(
        QSet<uint>() << 0,                                           // makesSenseForStatuses
        Features(),                                                  // dependsOnFeatures
        QStringList(),                                               // dependsOnInterfaces
        (ReadinessHelper::IntrospectFunc) &Private::introspectMain,
        this);
    introspectables[FeatureCore] = introspectableCore;

    readinessHelper = new ReadinessHelper(parent, 0 /* status */,
            introspectables, parent);
    readinessHelper->becomeReady(Features() << FeatureCore);

    init();
}

AccountManager::Private::~Private()
{
    delete baseInterface;
}

void AccountManager::Private::setAccountPaths(QSet<QString> &set,
        const QVariant &variant)
{
    Telepathy::ObjectPathList paths = qdbus_cast<Telepathy::ObjectPathList>(variant);

    if (paths.size() == 0) {
        // maybe the AccountManager is buggy, like Mission Control
        // 5.0.beta45, and returns an array of strings rather than
        // an array of object paths?

        QStringList wronglyTypedPaths = qdbus_cast<QStringList>(variant);
        if (wronglyTypedPaths.size() > 0) {
            warning() << "AccountManager returned wrong type "
                "(expected 'ao', got 'as'); working around it";
            foreach (QString path, wronglyTypedPaths) {
                set << path;
            }
        }
    }
    else {
        foreach (const QDBusObjectPath &path, paths) {
            set << path.path();
        }
    }
}

/**
 * \class AccountManager
 * \ingroup clientaccount
 * \headerfile TelepathyQt4/Client/account-manager.h> <TelepathyQt4/Client/AccountManager>
 *
 * Object representing a Telepathy account manager.
 */

const Feature AccountManager::FeatureCore = Feature(AccountManager::staticMetaObject.className(), 0);

/**
 * Construct a new AccountManager object.
 *
 * \param parent Object parent.
 */
AccountManager::AccountManager(QObject* parent)
    : StatelessDBusProxy(QDBusConnection::sessionBus(),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH), parent),
      OptionalInterfaceFactory<AccountManager>(this),
      mPriv(new Private(this))
{
}

/**
 * Construct a new AccountManager object.
 *
 * \param bus QDBusConnection to use.
 * \param parent Object parent.
 */
AccountManager::AccountManager(const QDBusConnection& bus,
        QObject* parent)
    : StatelessDBusProxy(bus,
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_BUS_NAME),
            QLatin1String(TELEPATHY_ACCOUNT_MANAGER_OBJECT_PATH), parent),
      OptionalInterfaceFactory<AccountManager>(this),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
AccountManager::~AccountManager()
{
    delete mPriv;
}

QStringList AccountManager::interfaces() const
{
    return mPriv->interfaces;
}

/**
 * \fn DBus::propertiesInterface *AccountManager::propertiesInterface() const
 *
 * Convenience function for getting a Properties interface proxy. The
 * AccountManager interface relies on properties, so this interface is
 * always assumed to be present.
 */

/**
 * Return a list of object paths for all valid accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::validAccountPaths() const
{
    return mPriv->validAccountPaths.values();
}

/**
 * Return a list of object paths for all invalid accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::invalidAccountPaths() const
{
    return mPriv->invalidAccountPaths.values();
}

/**
 * Return a list of object paths for all accounts.
 *
 * \return A list of object paths.
 */
QStringList AccountManager::allAccountPaths() const
{
    QStringList result;
    result.append(mPriv->validAccountPaths.values());
    result.append(mPriv->invalidAccountPaths.values());
    return result;
}

/**
 * Return a list of Account objects for all valid accounts.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using it.
 *
 * \return A list of Account objects
 * \sa invalidAccounts(), allAccounts(), accountsForPaths()
 */
QList<QSharedPointer<Account> > AccountManager::validAccounts()
{
    return accountsForPaths(validAccountPaths());
}

/**
 * Return a list of Account objects for all invalid accounts.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using it.
 *
 * \return A list of Account objects
 * \sa validAccounts(), allAccounts(), accountsForPaths()
 */
QList<QSharedPointer<Account> > AccountManager::invalidAccounts()
{
    return accountsForPaths(invalidAccountPaths());
}

/**
 * Return a list of Account objects for all accounts.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using it.
 *
 * \return A list of Account objects
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
QList<QSharedPointer<Account> > AccountManager::allAccounts()
{
    return accountsForPaths(allAccountPaths());
}

/**
 * Return an Account object for the given \a path.
 *
 * Note that the Account object won't be cached by account manager, and
 * should be done by the application itself.
 *
 * Remember to call Account::becomeReady on the new account, to
 * make sure it is ready before using it.
 *
 * \param path The object path to create account for.
 * \return A list of Account objects
 * \sa validAccounts(), invalidAccounts(), accountsForPaths()
 */
QSharedPointer<Account> AccountManager::accountForPath(const QString &path)
{
    if (mPriv->accounts.contains(path)) {
        return mPriv->accounts[path];
    }

    if (!mPriv->validAccountPaths.contains(path) &&
        !mPriv->invalidAccountPaths.contains(path)) {
        return QSharedPointer<Account>();
    }

    QSharedPointer<Account> account = QSharedPointer<Account>(
            new Account(this, path));
    mPriv->accounts[path] = account;
    return account;
}

/**
 * Return a list of Account objects for the given \a paths.
 *
 * Note that the Account objects won't be cached by account manager, and
 * should be done by the application itself.
 *
 * Remember to call Account::becomeReady on the new accounts, to
 * make sure they are ready before using it.
 *
 * \param paths List of object paths to create accounts for.
 * \return A list of Account objects
 * \sa validAccounts(), invalidAccounts(), allAccounts()
 */
QList<QSharedPointer<Account> > AccountManager::accountsForPaths(const QStringList &paths)
{
    QList<QSharedPointer<Account> > result;
    foreach (const QString &path, paths) {
        result << accountForPath(path);
    }
    return result;
}

/**
 * Create an Account with the given parameters.
 *
 * Return a pending operation representing the Account object which will succeed
 * when the account has been created or fail if an error occurred.
 *
 * \param connectionManager Name of the connection manager to create the account for.
 * \param protocol Name of the protocol to create the account for.
 * \param displayName Account display name.
 * \param parameters Account parameters.
 * \return A PendingOperation which will emit PendingAccount::finished
 *         when the account has been created of failed its creation process.
 */
PendingAccount *AccountManager::createAccount(const QString &connectionManager,
        const QString &protocol, const QString &displayName,
        const QVariantMap &parameters)
{
    return new PendingAccount(this, connectionManager,
            protocol, displayName, parameters);
}

/**
 * Return whether this object has finished its initial setup.
 *
 * This is mostly useful as a sanity check, in code that shouldn't be run
 * until the object is ready. To wait for the object to be ready, call
 * becomeReady() and connect to the finished signal on the result.
 *
 * \param features The features which should be tested
 * \return \c true if the object has finished its initial setup for basic
 *         functionality plus the given features
 */
bool AccountManager::isReady(const Features &features) const
{
    if (features.isEmpty()) {
        return mPriv->readinessHelper->isReady(Features() << FeatureCore, true);
    }
    return mPriv->readinessHelper->isReady(features, features.contains(FeatureCore));
}

/**
 * Return a pending operation which will succeed when this object finishes
 * its initial setup, or will fail if a fatal error occurs during this
 * initial setup.
 *
 * If an empty set is used FeatureCore will be considered as the requested
 * feature.
 *
 * \param requestedFeatures The features which should be enabled
 * \return A PendingReady object which will emit finished
 *         when this object has finished or failed initial setup for basic
 *         functionality plus the given features
 */
PendingReady *AccountManager::becomeReady(const Features &requestedFeatures)
{
    if (requestedFeatures.isEmpty()) {
        return mPriv->readinessHelper->becomeReady(Features() << FeatureCore);
    }
    return mPriv->readinessHelper->becomeReady(requestedFeatures);

}

Features AccountManager::requestedFeatures() const
{
    return mPriv->readinessHelper->requestedFeatures();
}

Features AccountManager::actualFeatures() const
{
    return mPriv->readinessHelper->actualFeatures();
}

Features AccountManager::missingFeatures() const
{
    return mPriv->readinessHelper->missingFeatures();
}

/**
 * Get the AccountManagerInterface for this AccountManager. This
 * method is protected since the convenience methods provided by this
 * class should generally be used instead of calling D-Bus methods
 * directly.
 *
 * \return A pointer to the existing AccountManagerInterface for this
 *         AccountManager.
 */
AccountManagerInterface *AccountManager::baseInterface() const
{
    return mPriv->baseInterface;
}

ReadinessHelper *AccountManager::readinessHelper() const
{
    return mPriv->readinessHelper;
}

/**** Private ****/
void AccountManager::Private::init()
{
    if (!parent->isValid()) {
        return;
    }

    parent->connect(baseInterface,
            SIGNAL(AccountValidityChanged(const QDBusObjectPath &, bool)),
            SLOT(onAccountValidityChanged(const QDBusObjectPath &, bool)));
    parent->connect(baseInterface,
            SIGNAL(AccountRemoved(const QDBusObjectPath &)),
            SLOT(onAccountRemoved(const QDBusObjectPath &)));
}

void AccountManager::Private::introspectMain(AccountManager::Private *self)
{
    DBus::PropertiesInterface *properties = self->parent->propertiesInterface();
    Q_ASSERT(properties != 0);

    debug() << "Calling Properties::GetAll(AccountManager)";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
            properties->GetAll(
                TELEPATHY_INTERFACE_ACCOUNT_MANAGER), self->parent);
    self->parent->connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher *)),
            SLOT(gotMainProperties(QDBusPendingCallWatcher *)));
}

void AccountManager::gotMainProperties(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap props;

    if (!reply.isError()) {
        debug() << "Got reply to Properties.GetAll(AccountManager)";
        props = reply.value();

        if (props.contains("Interfaces")) {
            mPriv->interfaces = qdbus_cast<QStringList>(props["Interfaces"]);
        }
        if (props.contains("ValidAccounts")) {
            mPriv->setAccountPaths(mPriv->validAccountPaths,
                    props["ValidAccounts"]);
        }
        if (props.contains("InvalidAccounts")) {
            mPriv->setAccountPaths(mPriv->invalidAccountPaths,
                    props["InvalidAccounts"]);
        }
    } else {
        warning().nospace() <<
            "GetAll(AccountManager) failed: " <<
            reply.error().name() << ": " << reply.error().message();
    }

    mPriv->readinessHelper->setIntrospectCompleted(FeatureCore, true);

    watcher->deleteLater();
}

void AccountManager::onAccountValidityChanged(const QDBusObjectPath &objectPath,
        bool nowValid)
{
    QString path = objectPath.path();
    bool newAccount = false;

    if (!mPriv->validAccountPaths.contains(path) &&
        !mPriv->invalidAccountPaths.contains(path)) {
        newAccount = true;
    }

    if (nowValid) {
        debug() << "Account created or became valid:" << path;
        mPriv->invalidAccountPaths.remove(path);
        mPriv->validAccountPaths.insert(path);
    }
    else {
        debug() << "Account became invalid:" << path;
        mPriv->validAccountPaths.remove(path);
        mPriv->invalidAccountPaths.insert(path);
    }

    if (newAccount) {
        emit accountCreated(path);
        // if the newly created account is invalid (shouldn't be the case)
        // emit also accountValidityChanged indicating this
        if (!nowValid) {
            emit accountValidityChanged(path, nowValid);
        }
    }
    else {
        emit accountValidityChanged(path, nowValid);
    }
}

void AccountManager::onAccountRemoved(const QDBusObjectPath &objectPath)
{
    QString path = objectPath.path();

    debug() << "Account removed:" << path;
    mPriv->validAccountPaths.remove(path);
    mPriv->invalidAccountPaths.remove(path);
    if (mPriv->accounts.contains(path)) {
        mPriv->accounts.remove(path);
    }
    emit accountRemoved(path);
}

} // Telepathy::Client
} // Telepathy
