#ifndef BLUETOOTHMONITOR_H
#define BLUETOOTHMONITOR_H

#include <QObject>
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

signals:
    void deviceConnected(const QString &macAddress, const QString &deviceName);
    void deviceDisconnected(const QString &macAddress, const QString &deviceName);
    void connectFinished(bool success, const QString &error);

private slots:
    void onPropertiesChanged(const QString &interface, const QVariantMap &changedProps, const QStringList &invalidatedProps);

private:
    QDBusConnection m_dbus;
    void registerDBusService();
    bool isAirPodsDevice(const QString &devicePath);
    QString getDeviceName(const QString &devicePath);
};

#endif // BLUETOOTHMONITOR_H