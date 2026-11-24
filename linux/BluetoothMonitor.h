#ifndef BLUETOOTHMONITOR_H
#define BLUETOOTHMONITOR_H

#include <QObject>
#include <QSet>
#include <QtDBus/QtDBus>

// Forward declarations for D-Bus types
typedef QMap<QDBusObjectPath, QMap<QString, QVariantMap>> ManagedObjectList;
Q_DECLARE_METATYPE(ManagedObjectList)

class BluetoothMonitor : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    explicit BluetoothMonitor(QObject *parent = nullptr);
    ~BluetoothMonitor();

    bool checkAlreadyConnectedDevices();
    // Ask BlueZ to connect paired AirPods (preferring preferredAddress). Returns
    // false if no AirPods are paired; otherwise connectFinished is emitted later.
    bool connectPairedAirPods(const QString &preferredAddress = QString());
    // Ask BlueZ to disconnect the device with this address (all profiles, audio
    // too). Returns false if BlueZ doesn't know it; otherwise disconnectFinished
    // is emitted later.
    bool disconnectDevice(const QString &address);
    // Paired AirPods known to BlueZ, as {address, name} maps sorted by name.
    QVariantList pairedAirPods();

signals:
    void deviceConnected(const QString &macAddress, const QString &deviceName);
    void deviceDisconnected(const QString &macAddress, const QString &deviceName);
    void connectFinished(bool success, const QString &error);
    void disconnectFinished(bool success, const QString &error);
    // A device was paired, unpaired, renamed or removed: re-read pairedAirPods().
    void pairedAirPodsChanged();

private slots:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changedProps, const QStringList &invalidatedProps);
    void onInterfacesRemoved(const QDBusObjectPath &path, const QStringList &interfaces);

private:
    QDBusConnection m_dbus;
    // Connected devices whose services BlueZ hadn't read yet when they connected
    // (a first pairing), so they couldn't be identified as AirPods then.
    QSet<QString> m_awaitingServices;
    void registerDBusService();
    bool isAirPodsDevice(const QString &devicePath);
    QString getDeviceName(const QString &devicePath);
    QString devicePathForAddress(const QString &address);
    void announceConnection(const QString &devicePath, bool connected);
};

#endif // BLUETOOTHMONITOR_H