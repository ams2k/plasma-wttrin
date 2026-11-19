#pragma once

#include <plasma/weather/ion.h>

#include <QNetworkAccessManager>

class WttrInIon final : public Ion
{
    Q_OBJECT

public:
    explicit WttrInIon(QObject *parent = nullptr);
    ~WttrInIon() override = default;

public Q_SLOTS:
    void findPlaces(std::shared_ptr<QPromise<std::shared_ptr<Locations>>> promise,
                    const QString &searchString) override;

    void fetchForecast(std::shared_ptr<QPromise<std::shared_ptr<Forecast>>> promise,
                       const QString &placeInfo) override;

private:
    QNetworkAccessManager m_network;

    static QUrl makeUrl(const QString &place, const QString &query);
};
