#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QThread>

struct Track {
private:
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name);

public:
    QString name;
};
Q_DECLARE_METATYPE(Track)

class TrackLoaderWorker : public QObject {
    Q_OBJECT

public slots:
    void load(const QString& fileName)
    {
        QString result;
        QList<Track> list;
        for (int i = 0; i < 100000; ++i) {
            Track track;
            track.name = QString("%1%2").arg(fileName).arg(i);
            list.append(std::move(track));
        }
        emit loaded(list);
    }

signals:
    void loaded(const QList<Track>& list);
};

class TrackLoader : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading WRITE setLoading NOTIFY loadingChanged)
    QThread m_thread;
    bool m_loading = false;

public:
    TrackLoader()
    {
        TrackLoaderWorker* worker = new TrackLoaderWorker;
        worker->moveToThread(&m_thread);
        connect(&m_thread, &QThread::finished, worker, &QObject::deleteLater);
        connect(this, &TrackLoader::doload, worker, &TrackLoaderWorker::load);
        connect(this, &TrackLoader::doload, this, [this](auto&&) {
            setLoading(true);
        });
        connect(worker, &TrackLoaderWorker::loaded, this, &TrackLoader::loaded);
        connect(worker, &TrackLoaderWorker::loaded, this, [this](auto&&) {
            setLoading(false);
        });
        m_thread.start();
    }
    ~TrackLoader()
    {
        m_thread.quit();
        m_thread.wait();
    }
    bool loading() const
    {
        return m_loading;
    }

signals:
    void doload(const QString&);
    void loaded(const QList<Track>& list);

    void loadingChanged(bool loading);

public slots:
    Q_INVOKABLE void load(const QString& fileName) { emit doload(fileName); }
    void setLoading(bool loading)
    {
        if (m_loading == loading)
            return;

        m_loading = loading;
        emit loadingChanged(m_loading);
    }
};

class TrackModel : public QAbstractListModel {
    Q_OBJECT

    enum { TRACK_ROLE = Qt::UserRole + 1,
        TRACK_NAME_ROLE };

public:
    virtual int rowCount(const QModelIndex& parent) const override
    {
        if (parent.isValid()) {
            return 0;
        }
        return m_list.size();
    }
    virtual QVariant data(const QModelIndex& index, int role) const override
    {
        if (index.isValid()) {
            switch (role) {
            case TRACK_ROLE:
                return QVariant::fromValue(m_list.at(index.row()));
            case TRACK_NAME_ROLE:
                return m_list.at(index.row()).name;
            }
        }
        return QVariant();
    }
    virtual QHash<int, QByteArray> roleNames() const override
    {
        static QHash<int, QByteArray> roles {
            std::make_pair(TRACK_ROLE, QByteArray("track")),
            std::make_pair(TRACK_NAME_ROLE, QByteArray("name")),
        };
        return roles;
    }

public:
    Q_INVOKABLE void setList(QList<Track> list)
    {
        beginResetModel();
        m_list = list;
        endResetModel();
    }

private:
    QList<Track> m_list;
};

int main(int argc, char* argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    qRegisterMetaType<QList<Track>>();
    qmlRegisterType<TrackLoader>("myqml", 1, 0, "TrackLoader");
    qmlRegisterType<TrackModel>("myqml", 1, 0, "TrackModel");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

#include "main.moc"
