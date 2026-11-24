#include "BluetoothMonitor.h"
#include "logger.h"

#include <QDebug>
#include <algorithm>
#include <QDBusObjectPath>
#include <QDBusMetaType>

BluetoothMonitor::BluetoothMonitor(QObject *parent)
    : QObject(parent), m_dbus(QDBusConnection::systemBus())
{
    // Register meta-types for D-Bus interaction
    qDBusRegisterMetaType<QDBusObjectPath>();
    qDBusRegisterMetaType<ManagedObjectList>();

    if (!m_dbus.isConnected())
    {
        LOG_WARN("Failed to connect to system D-Bus");
        return;
    }

    registerDBusService();
    checkAlreadyConnectedDevices(); // Check for already connected devices on startup
}

BluetoothMonitor::~BluetoothMonitor()
{
    m_dbus.disconnectFromBus(m_dbus.name());
}

void BluetoothMonitor::registerDBusService()
{
    // Match signals for PropertiesChanged on any BlueZ Device interface
    if (!m_dbus.connect("", "", "org.freedesktop.DBus.Properties", "PropertiesChanged",
                        this, SLOT(onPropertiesChanged(QString, QVariantMap, QStringList))))
    {
        LOG_WARN("Failed to connect to D-Bus PropertiesChanged signal");
    }
    // Device objects go away when a device is removed (unpaired)
    if (!m_dbus.connect("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved",
                        this, SLOT(onInterfacesRemoved(QDBusObjectPath, QStringList))))
    {
        LOG_WARN("Failed to connect to D-Bus InterfacesRemoved signal");
    }
}

void BluetoothMonitor::onInterfacesRemoved(const QDBusObjectPath &path, const QStringList &interfaces)
{
    Q_UNUSED(path);
    if (interfaces.contains("org.bluez.Device1"))
    {
        emit pairedAirPodsChanged();
    }
}

bool BluetoothMonitor::isAirPodsDevice(const QString &devicePath)
{
    QDBusInterface deviceInterface("org.bluez", devicePath, "org.freedesktop.DBus.Properties", m_dbus);

    // Get UUIDs to check if it's an AirPods device
    QDBusReply<QVariant> uuidsReply = deviceInterface.call("Get", "org.bluez.Device1", "UUIDs");
    if (!uuidsReply.isValid())
    {
        return false;
    }

    QStringList uuids = uuidsReply.value().toStringList();
    return uuids.contains("74ec2172-0bad-4d01-8f77-997b2be0722a");
}

QString BluetoothMonitor::getDeviceName(const QString &devicePath)
{
    QDBusInterface deviceInterface("org.bluez", devicePath, "org.freedesktop.DBus.Properties", m_dbus);
    QDBusReply<QVariant> nameReply = deviceInterface.call("Get", "org.bluez.Device1", "Name");
    if (nameReply.isValid())
    {
        return nameReply.value().toString();
    }
    return "Unknown";
}

bool BluetoothMonitor::checkAlreadyConnectedDevices()
{
    QDBusInterface objectManager("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", m_dbus);
    QDBusMessage reply = objectManager.call("GetManagedObjects");

    if (reply.type() == QDBusMessage::ErrorMessage)
    {
        LOG_WARN("Failed to get managed objects: " << reply.errorMessage());
        return false;
    }

    QVariant firstArg = reply.arguments().constFirst();
    QDBusArgument arg = firstArg.value<QDBusArgument>();
    ManagedObjectList managedObjects;
    arg >> managedObjects;

    bool deviceFound = false;

    for (auto it = managedObjects.constBegin(); it != managedObjects.constEnd(); ++it)
    {
        const QDBusObjectPath &objPath = it.key();
        const QMap<QString, QVariantMap> &interfaces = it.value();

        if (interfaces.contains("org.bluez.Device1"))
        {
            const QVariantMap &deviceProps = interfaces.value("org.bluez.Device1");

            // Check if the device has the necessary properties
            if (!deviceProps.contains("UUIDs") || !deviceProps.contains("Connected") ||
                !deviceProps.contains("Address") || !deviceProps.contains("Name"))
            {
                continue;
            }

            QStringList uuids = deviceProps["UUIDs"].toStringList();
            bool isAirPods = uuids.contains("74ec2172-0bad-4d01-8f77-997b2be0722a");

            if (isAirPods)
            {
                bool connected = deviceProps["Connected"].toBool();
                if (connected)
                {
                    QString macAddress = deviceProps["Address"].toString();
                    QString deviceName = deviceProps["Name"].toString();
                    emit deviceConnected(macAddress, deviceName);
                    LOG_DEBUG("Found already connected AirPods: " << macAddress << " Name: " << deviceName);
                    deviceFound = true;
                }
            }
        }
    }
    return deviceFound;
}

bool BluetoothMonitor::connectPairedAirPods(const QString &preferredAddress)
{
    QDBusInterface objectManager("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", m_dbus);
    QDBusMessage reply = objectManager.call("GetManagedObjects");
    if (reply.type() == QDBusMessage::ErrorMessage)
    {
        LOG_WARN("Failed to get managed objects: " << reply.errorMessage());
        return false;
    }

    ManagedObjectList managedObjects;
    reply.arguments().constFirst().value<QDBusArgument>() >> managedObjects;

    QString devicePath, address, name;
    bool alreadyConnected = false;
    for (auto it = managedObjects.constBegin(); it != managedObjects.constEnd(); ++it)
    {
        const QVariantMap props = it.value().value("org.bluez.Device1");
        if (!props.value("Paired").toBool() ||
            !props.value("UUIDs").toStringList().contains("74ec2172-0bad-4d01-8f77-997b2be0722a"))
            continue;

        const QString candidate = props.value("Address").toString();
        if (devicePath.isEmpty() || candidate.compare(preferredAddress, Qt::CaseInsensitive) == 0)
        {
            devicePath = it.key().path();
            address = candidate;
            name = props.value("Alias", props.value("Name")).toString();
            alreadyConnected = props.value("Connected").toBool();
        }
    }

    if (devicePath.isEmpty())
    {
        LOG_WARN("No paired AirPods found");
        return false;
    }

    if (alreadyConnected)
    {
        // BlueZ is connected but the app isn't; run the normal connect handling.
        LOG_INFO("AirPods already connected in BlueZ: " << address);
        emit deviceConnected(address, name);
        emit connectFinished(true, QString());
        return true;
    }

    LOG_INFO("Connecting to paired AirPods: " << name << " (" << address << ")");
    QDBusInterface device("org.bluez", devicePath, "org.bluez.Device1", m_dbus);
    auto *watcher = new QDBusPendingCallWatcher(device.asyncCall("Connect"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call)
    {
        const QDBusPendingReply<> result = *call;
        if (result.isError())
        {
            LOG_WARN("Failed to connect AirPods: " << result.error().message());
            emit connectFinished(false, result.error().message());
        }
        else
        {
            emit connectFinished(true, QString());
        }
        call->deleteLater();
    });
    return true;
}

QVariantList BluetoothMonitor::pairedAirPods()
{
    QDBusInterface objectManager("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", m_dbus);
    QDBusMessage reply = objectManager.call("GetManagedObjects");
    if (reply.type() == QDBusMessage::ErrorMessage)
    {
        LOG_WARN("Failed to get managed objects: " << reply.errorMessage());
        return {};
    }

    ManagedObjectList managedObjects;
    reply.arguments().constFirst().value<QDBusArgument>() >> managedObjects;

    QList<QVariantMap> devices;
    for (auto it = managedObjects.constBegin(); it != managedObjects.constEnd(); ++it)
    {
        const QVariantMap props = it.value().value("org.bluez.Device1");
        if (!props.value("Paired").toBool() ||
            !props.value("UUIDs").toStringList().contains("74ec2172-0bad-4d01-8f77-997b2be0722a"))
            continue;
        devices.append({{"address", props.value("Address").toString()},
                        {"name", props.value("Alias", props.value("Name")).toString()}});
    }
    std::sort(devices.begin(), devices.end(), [](const QVariantMap &a, const QVariantMap &b)
              { return a.value("name").toString().localeAwareCompare(b.value("name").toString()) < 0; });

    QVariantList result;
    for (const QVariantMap &device : devices)
        result.append(device);
    return result;
}

QString BluetoothMonitor::devicePathForAddress(const QString &address)
{
    QDBusInterface objectManager("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", m_dbus);
    QDBusMessage reply = objectManager.call("GetManagedObjects");
    if (reply.type() == QDBusMessage::ErrorMessage)
    {
        LOG_WARN("Failed to get managed objects: " << reply.errorMessage());
        return QString();
    }

    ManagedObjectList managedObjects;
    reply.arguments().constFirst().value<QDBusArgument>() >> managedObjects;

    for (auto it = managedObjects.constBegin(); it != managedObjects.constEnd(); ++it)
    {
        const QVariantMap props = it.value().value("org.bluez.Device1");
        if (props.value("Address").toString().compare(address, Qt::CaseInsensitive) == 0)
        {
            return it.key().path();
        }
    }
    return QString();
}

bool BluetoothMonitor::disconnectDevice(const QString &address)
{
    const QString devicePath = devicePathForAddress(address);
    if (devicePath.isEmpty())
    {
        LOG_WARN("Device not known to BlueZ: " << address);
        return false;
    }

    LOG_INFO("Disconnecting AirPods: " << address);
    QDBusInterface device("org.bluez", devicePath, "org.bluez.Device1", m_dbus);
    auto *watcher = new QDBusPendingCallWatcher(device.asyncCall("Disconnect"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call)
    {
        const QDBusPendingReply<> result = *call;
        if (result.isError())
        {
            LOG_WARN("Failed to disconnect AirPods: " << result.error().message());
            emit disconnectFinished(false, result.error().message());
        }
        else
        {
            emit disconnectFinished(true, QString());
        }
        call->deleteLater();
    });
    return true;
}

void BluetoothMonitor::onPropertiesChanged(const QString &interface, const QVariantMap &changedProps, const QStringList &invalidatedProps)
{
    Q_UNUSED(invalidatedProps);

    if (interface != "org.bluez.Device1")
    {
        return;
    }

    QString path = QDBusContext::message().path();

    // UUIDs: a new pairing only shows up as AirPods once its services are read
    if (changedProps.contains("Paired") || changedProps.contains("Alias") || changedProps.contains("UUIDs"))
    {
        emit pairedAirPodsChanged();
    }

    if (changedProps.contains("Connected"))
    {
        bool connected = changedProps["Connected"].toBool();
        m_awaitingServices.remove(path);

        if (!isAirPodsDevice(path))
        {
            // When AirPods are first paired, BlueZ connects before it has read
            // their services, so the AirPods UUID isn't listed yet. Look again
            // once the services arrive (below).
            if (connected)
            {
                m_awaitingServices.insert(path);
            }
            return;
        }

        announceConnection(path, connected);
    }
    else if (m_awaitingServices.contains(path) &&
             (changedProps.contains("UUIDs") || changedProps.contains("ServicesResolved")))
    {
        if (!isAirPodsDevice(path))
        {
            return;
        }
        // Still connected: a disconnect would have removed it from the set.
        m_awaitingServices.remove(path);
        LOG_INFO("AirPods services resolved after connecting (new pairing)");
        announceConnection(path, true);
    }
}

void BluetoothMonitor::announceConnection(const QString &devicePath, bool connected)
{
    QDBusInterface deviceInterface("org.bluez", devicePath, "org.freedesktop.DBus.Properties", m_dbus);

    // Get the device address
    QDBusReply<QVariant> addrReply = deviceInterface.call("Get", "org.bluez.Device1", "Address");
    if (!addrReply.isValid())
    {
        return;
    }
    QString macAddress = addrReply.value().toString();
    QString deviceName = getDeviceName(devicePath);

    if (connected)
    {
        emit deviceConnected(macAddress, deviceName);
        LOG_DEBUG("AirPods device connected:" << macAddress << " Name:" << deviceName);
    }
    else
    {
        emit deviceDisconnected(macAddress, deviceName);
        LOG_DEBUG("AirPods device disconnected:" << macAddress << " Name:" << deviceName);
    }
}