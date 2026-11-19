#include "wttrin.h"

#include <KPluginFactory>
#include <KUnitConversion/Unit>

#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStringList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointF>
#include <QUrlQuery>

#include <algorithm>

K_PLUGIN_CLASS_WITH_JSON(WttrInIon, "plasma-wttrin.json")

namespace
{
QString jsonText(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        return array.isEmpty() ? QString() : array.first().toObject().value("value").toString();
    }
    return value.toString();
}

qreal jsonNumber(const QJsonObject &object, const QString &key, qreal fallback = 0.0)
{
    bool ok = false;
    const qreal value = object.value(key).toString().toDouble(&ok);
    if (ok)
        return value;

    const QJsonValue raw = object.value(key);
    if (raw.isDouble())
        return raw.toDouble(fallback);
    return fallback;
}

QString conditionIcon(const QString &code, bool night = false)
{
    // Códigos de condição do serviço wttr.in (WeatherAPI/World Weather Online).
    const int value = code.toInt();
    QString base;

    if (value == 113) base = "weather-clear";
    else if (value == 116) base = "weather-few-clouds";
    else if (value == 119 || value == 122) base = "weather-overcast";
    else if (value == 143 || value == 248 || value == 260) base = "weather-fog";
    else if (value == 176 || value == 263 || value == 266 ||
             value == 293 || value == 296 || value == 353) base = "weather-showers";
    else if (value == 200 || value == 386 || value == 389 ||
             value == 392 || value == 395) base = "weather-storm";
    else if (value == 179 || value == 182 || value == 185 ||
             value == 227 || value == 230 || value == 281 ||
             value == 284 || value == 311 || value == 314 ||
             value == 317 || value == 320 || value == 323 ||
             value == 326 || value == 329 || value == 332 ||
             value == 335 || value == 338 || value == 350 ||
             value == 362 || value == 365 || value == 368 ||
             value == 371 || value == 374 || value == 377) base = "weather-snow";
    else if (value == 299 || value == 302 || value == 305 ||
             value == 308 || value == 356 || value == 359) base = "weather-rain";
    else base = "weather-none-available";

    if (night && base == "weather-clear")
        return "weather-clear-night";
    if (night && base == "weather-few-clouds")
        return "weather-few-clouds-night";
    return base;
}

QString areaDisplayName(const QJsonObject &area)
{
    const QString city = jsonText(area, "areaName");
    const QString region = jsonText(area, "region");
    const QString country = jsonText(area, "country");

    QStringList parts;
    if (!city.isEmpty()) parts << city;
    if (!region.isEmpty() && region.compare(city, Qt::CaseInsensitive) != 0)
        parts << region;
    if (!country.isEmpty()) parts << country;
    return parts.join(", ");
}

QJsonObject firstObject(const QJsonArray &array)
{
    return array.isEmpty() ? QJsonObject() : array.first().toObject();
}

void finishLocations(const std::shared_ptr<QPromise<std::shared_ptr<Locations>>> &promise,
                     const std::shared_ptr<Locations> &locations)
{
    promise->addResult(locations);
    promise->finish();
}

void finishForecast(const std::shared_ptr<QPromise<std::shared_ptr<Forecast>>> &promise,
                    const std::shared_ptr<Forecast> &forecast)
{
    promise->addResult(forecast);
    promise->finish();
}
}

WttrInIon::WttrInIon(QObject *parent)
    : Ion(parent)
{
}

QUrl WttrInIon::makeUrl(const QString &place, const QString &query)
{
    // wttr.in responde JSON em j1; lang=pt solicita descrições em português.
    QUrl url(QStringLiteral("https://wttr.in/") + QString::fromUtf8(QUrl::toPercentEncoding(place)));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("format"), QStringLiteral("j1"));
    urlQuery.addQueryItem(QStringLiteral("lang"), QStringLiteral("pt-br"));
    if (!query.isEmpty())
        urlQuery.addQueryItem(QStringLiteral("q"), query);
    url.setQuery(urlQuery);
    return url;
}

void WttrInIon::findPlaces(
    std::shared_ptr<QPromise<std::shared_ptr<Locations>>> promise,
    const QString &searchString)
{
    auto locations = std::make_shared<Locations>();

    if (searchString.trimmed().isEmpty()) {
        locations->setError();
        finishLocations(promise, locations);
        return;
    }

    QNetworkRequest request(makeUrl(searchString.trimmed(), QString()));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("plasma-wttrin/0.1 (KDE Plasma Weather provider)"));
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this,
            [reply, promise, locations]() {
        const QByteArray payload = reply->readAll();
        const bool failed = reply->error() != QNetworkReply::NoError;
        reply->deleteLater();

        if (failed) {
            locations->setError();
            finishLocations(promise, locations);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            locations->setError();
            finishLocations(promise, locations);
            return;
        }

        const QJsonObject root = document.object();
        const QJsonArray areas = root.value("nearest_area").toArray();
        if (areas.isEmpty()) {
            locations->setError();
            finishLocations(promise, locations);
            return;
        }

        for (const QJsonValue &value : areas) {
            const QJsonObject area = value.toObject();
            const QString display = areaDisplayName(area);
            const QJsonObject latitude = firstObject(area.value("latitude").toArray());
            const QJsonObject longitude = firstObject(area.value("longitude").toArray());

            Location location;
            location.setPlaceInfo(display.isEmpty() ? QStringLiteral("São Paulo") : display);
            location.setDisplayName(display);
            location.setStation(jsonText(area, "areaName"));
            bool latOk = false;
            bool lonOk = false;
            const qreal lat = jsonText(area, "latitude").toDouble(&latOk);
            const qreal lon = jsonText(area, "longitude").toDouble(&lonOk);
            Q_UNUSED(latitude);
            Q_UNUSED(longitude);
            if (latOk && lonOk)
                location.setCoordinates(QPointF(lon, lat));
            locations->addLocation(location);
        }

        finishLocations(promise, locations);
    });
}

void WttrInIon::fetchForecast(
    std::shared_ptr<QPromise<std::shared_ptr<Forecast>>> promise,
    const QString &placeInfo)
{
    auto forecast = std::make_shared<Forecast>();

    QNetworkRequest request(makeUrl(placeInfo.trimmed(), QString()));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("plasma-wttrin/0.1 (KDE Plasma Weather provider)"));
    request.setTransferTimeout(15000);

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this,
            [reply, promise, forecast, placeInfo]() {
        const QByteArray payload = reply->readAll();
        const bool failed = reply->error() != QNetworkReply::NoError;
        reply->deleteLater();

        if (failed) {
            forecast->setError();
            finishForecast(promise, forecast);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            forecast->setError();
            finishForecast(promise, forecast);
            return;
        }

        const QJsonObject root = document.object();
        const QJsonObject current = firstObject(root.value("current_condition").toArray());
        const QJsonArray days = root.value("weather").toArray();
        const QJsonObject area = firstObject(root.value("nearest_area").toArray());

        if (current.isEmpty() || days.isEmpty()) {
            forecast->setError();
            finishForecast(promise, forecast);
            return;
        }

        MetaData metadata;
        metadata.setCredit(QStringLiteral("Dados meteorológicos: wttr.in"));
        metadata.setCreditURL(QStringLiteral("https://wttr.in/"));
        metadata.setTemperatureUnit(KUnitConversion::Celsius);
        metadata.setWindSpeedUnit(KUnitConversion::KilometerPerHour);
        metadata.setVisibilityUnit(KUnitConversion::Kilometer);
        metadata.setPressureUnit(KUnitConversion::Millibar);
        metadata.setHumidityUnit(KUnitConversion::Percent);
        metadata.setRainfallUnit(KUnitConversion::Millimeter);
        metadata.setSnowfallUnit(KUnitConversion::Millimeter);
        metadata.setPrecipUnit(KUnitConversion::Millimeter);
        forecast->setMetadata(metadata);

        Station station;
        station.setStation(jsonText(area, "areaName"));
        station.setPlace(jsonText(area, "areaName"));
        station.setRegion(jsonText(area, "region"));
        station.setCountry(jsonText(area, "country"));
        station.setNewPlaceInfo(placeInfo);
        bool latOk = false;
        bool lonOk = false;
        const qreal lat = jsonText(area, "latitude").toDouble(&latOk);
        const qreal lon = jsonText(area, "longitude").toDouble(&lonOk);
        if (latOk && lonOk)
            station.setCoordinates(lat, lon);
        forecast->setStation(station);

        LastObservation observation;
        observation.setObservationTimestamp(QDateTime::currentDateTime());
        observation.setCurrentConditions(jsonText(firstObject(current.value("weatherDesc").toArray()), "value"));
        observation.setTemperature(jsonNumber(current, "temp_C"));
        observation.setWindchill(jsonNumber(current, "FeelsLikeC"));
        observation.setWindSpeed(jsonNumber(current, "windspeedKmph"));
        observation.setWindGust(jsonNumber(current, "windspeedKmph"));
        observation.setWindDirection(jsonText(current, "winddir16Point"));
        observation.setVisibility(jsonNumber(current, "visibility"));
        observation.setPressure(jsonNumber(current, "pressure"));
        observation.setHumidity(jsonNumber(current, "humidity"));
        observation.setDewpoint(jsonNumber(current, "DewPointC"));
        observation.setUVIndex(static_cast<int>(jsonNumber(current, "uvIndex")));
        observation.setConditionIcon(conditionIcon(jsonText(current, "weatherCode")));
        forecast->setLastObservation(observation);

        auto futureDays = std::make_shared<FutureDays>();
        for (const QJsonValue &dayValue : days) {
            const QJsonObject day = dayValue.toObject();
            const QDate date = QDate::fromString(day.value("date").toString(), Qt::ISODate);
            const QJsonArray hourly = day.value("hourly").toArray();

            FutureDayForecast dayForecast;
            if (date.isValid()) {
                dayForecast.setMonthDay(date.day());
                dayForecast.setWeekDay(QLocale(QLocale::Portuguese, QLocale::Brazil)
                                           .dayName(date.dayOfWeek(), QLocale::ShortFormat));
            }

            if (!hourly.isEmpty()) {
                const QJsonObject morning = hourly.at(hourly.size() / 3).toObject();
                const QJsonObject evening = hourly.last().toObject();

                FutureForecast daytime;
                daytime.setCondition(jsonText(firstObject(morning.value("weatherDesc").toArray()), "value"));
                daytime.setConditionIcon(conditionIcon(jsonText(morning, "weatherCode")));
                daytime.setHighTemp(jsonNumber(day, "maxtempC"));
                daytime.setLowTemp(jsonNumber(day, "mintempC"));
                daytime.setConditionProbability(jsonNumber(morning, "chanceofrain"));
                dayForecast.setDaytime(daytime);

                FutureForecast night;
                night.setCondition(jsonText(firstObject(evening.value("weatherDesc").toArray()), "value"));
                night.setConditionIcon(conditionIcon(jsonText(evening, "weatherCode"), true));
                night.setHighTemp(jsonNumber(day, "maxtempC"));
                night.setLowTemp(jsonNumber(day, "mintempC"));
                night.setConditionProbability(jsonNumber(evening, "chanceofrain"));
                dayForecast.setNight(night);
            }
            futureDays->addDay(dayForecast);
        }
        forecast->setFutureDays(futureDays);

        finishForecast(promise, forecast);
    });
}

#include "wttrin.moc"
