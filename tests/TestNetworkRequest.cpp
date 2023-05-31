#include "TestNetworkRequest.h"
#include "core/NetworkManager.h"
#include "core/NetworkRequest.h"
#include "mock/MockNetworkAccessManager.h"
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

QTEST_GUILESS_MAIN(TestNetworkRequest)

using ContentTypeParameters_t = QHash<QString, QString>;
Q_DECLARE_METATYPE(ContentTypeParameters_t);
Q_DECLARE_METATYPE(std::chrono::milliseconds);

static constexpr auto TIMEOUT_GRACE_MS = 25;

void TestNetworkRequest::testNetworkRequest()
{
    QFETCH(QUrl, requestedURL);
    QFETCH(QUrl, expectedURL);
    QFETCH(QByteArray, expectedContent);
    QFETCH(QString, responseContentType);
    QFETCH(QString, expectedContentType);
    QFETCH(ContentTypeParameters_t, expectedContentTypeParameters);
    QFETCH(QString, expectedUserAgent);
    QFETCH(bool, expectError);
    QFETCH(QNetworkReply::NetworkError, error);

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    auto& reply = manager
        .whenGet(expectedURL)
        // Has right user agent?
        .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                           QRegularExpression(expectedUserAgent)))
        .reply();
    if (!expectError) {
        reply.withBody(expectedContent).withHeader(QNetworkRequest::ContentTypeHeader, responseContentType);
    } else {
        reply.withError(error);
    }

    // Create request
    NetworkRequest request = buildRequest(requestedURL).setManager(&manager).build();

    QString actualContent;
    bool didError = false, didSucceed = false;

    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&actualContent, &didSucceed](QByteArray content) {
        actualContent = QString(content);
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });

    request.fetch();
    QTest::qWait(300);

    // Ensures that predicates match - i.e., the header was set correctly
    QCOMPARE(manager.matchedRequests().length(), 1);
    QCOMPARE(request.URL(), expectedURL);
    if(!expectError) {
        QCOMPARE(actualContent, expectedContent);
        QCOMPARE(request.ContentType(), expectedContentType);
        QCOMPARE(request.ContentTypeParameters(), expectedContentTypeParameters);
        QCOMPARE(didSucceed, true);
        QCOMPARE(didError, false);
        QCOMPARE(request.Reply()->isFinished(), true);
    } else {
        QCOMPARE(didSucceed, false);
        QCOMPARE(didError, true);
    }
}
void TestNetworkRequest::testNetworkRequest_data()
{
    QTest::addColumn<QUrl>("requestedURL");
    QTest::addColumn<QUrl>("expectedURL");
    QTest::addColumn<QByteArray>("expectedContent");
    QTest::addColumn<QString>("responseContentType");
    QTest::addColumn<QString>("expectedContentType");
    QTest::addColumn<ContentTypeParameters_t>("expectedContentTypeParameters");
    QTest::addColumn<QString>("expectedUserAgent");
    QTest::addColumn<bool>("expectError");
    QTest::addColumn<QNetworkReply::NetworkError>("error");

    QString defaultUserAgent("KeePassXC");

    //const QUrl& exampleURL = QUrl{"https://example.com"};
    const QUrl& exampleURL = QUrl{"https://example.com"};
    const QByteArray& exampleContent = QString{"test-content"}.toUtf8();

    QTest::newRow("successful request") << exampleURL << exampleURL << exampleContent << "text/plain"
                                        << "text/plain" << ContentTypeParameters_t{}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("content type") << exampleURL << exampleURL << exampleContent << "application/test-content-type"
                                        << "application/test-content-type" << ContentTypeParameters_t{}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("empty content type") << exampleURL << exampleURL << QByteArray{} << "" << "" << ContentTypeParameters_t{}
                                  << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("content type parameters") << exampleURL << exampleURL << exampleContent << "application/test-content-type;test-param=test-value"
                                        << "application/test-content-type" << ContentTypeParameters_t {{"test-param", "test-value"}}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("content type parameters trimmed") << exampleURL << exampleURL << exampleContent << "application/test-content-type; test-param = test-value"
                                        << "application/test-content-type" << ContentTypeParameters_t {{"test-param", "test-value"}}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("request without schema should add https") << QUrl("example.com") << QUrl("https://example.com") << exampleContent << "text/plain"
                                                             << "text/plain" << ContentTypeParameters_t{}
                                                             << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
    QTest::newRow("request without schema should add https (edge case with // but no scheme)") << QUrl("//example.com") << QUrl("https://example.com") << exampleContent << "text/plain"
                                        << "text/plain" << ContentTypeParameters_t{}
                                        << defaultUserAgent << false << QNetworkReply::NetworkError::NoError;
}

void TestNetworkRequest::testNetworkRequestTimeout()
{
    // Timeout should work for single request
    // Timeout should capture entire duration, including redirects
    QFETCH(bool, expectError);
    QFETCH(std::chrono::milliseconds, delay);
    QFETCH(std::chrono::milliseconds, timeout);

    const auto requestedURL = QUrl("https://example.com");
    const auto expectedUserAgent = QString("KeePassXC");

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    auto& reply = manager
        .whenGet(requestedURL)
            // Has right user agent?
        .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                           QRegularExpression(expectedUserAgent)))
        .reply();

    // Timeout
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(delay);

    reply.withFinishDelayUntil(&timer, &QTimer::timeout);

    // Create request
    NetworkRequest request = buildRequest(requestedURL).setManager(&manager).setTimeout(timeout).build();

    // Start timer
    timer.start();

    bool didSucceed = false, didError = false;
    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&didSucceed](const QByteArray&) {
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });

    request.fetch();
    // Wait until the timeout should have (or not) occured
    QTest::qWait(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count() + TIMEOUT_GRACE_MS);

    QTEST_ASSERT(didError || didSucceed);

    // Ensures that predicates match - i.e., the header was set correctly
    QCOMPARE(manager.matchedRequests().length(), 1);
    QCOMPARE(request.URL(), requestedURL);
    QCOMPARE(didSucceed, !expectError);
    QCOMPARE(didError, expectError);
}

void TestNetworkRequest::testNetworkRequestTimeout_data()
{

    QTest::addColumn<bool>("expectError");
    QTest::addColumn<std::chrono::milliseconds>("delay");
    QTest::addColumn<std::chrono::milliseconds>("timeout");

    QTest::newRow("timeout") << true << std::chrono::milliseconds{100} << std::chrono::milliseconds{50};
    QTest::newRow("no timeout") << false << std::chrono::milliseconds{50} << std::chrono::milliseconds{100};
}

void TestNetworkRequest::testNetworkRequestRedirects()
{
    // Should respect max number of redirects
    // Headers, Reply, etc. should reflect final request

    QFETCH(int, numRedirects);
    QFETCH(int, maxRedirects);
    const bool expectError = numRedirects > maxRedirects;

    const auto requestedURL = QUrl("https://example.com");
    const auto expectedUserAgent = QString("KeePassXC");

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    QStringList requestedUrls;

    auto* reply = &manager
        .whenGet(requestedURL)
            // Has right user agent?
        .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                           QRegularExpression(expectedUserAgent)))
        .reply();

    for(int i = 0; i < numRedirects; ++i) {
        auto redirectTarget = QUrl("https://example.com/redirect" + QString::number(i));
        reply->withRedirect(redirectTarget);
        reply = &manager.whenGet(redirectTarget)
            // Has right user agent?
            .has(MockNetworkAccess::Predicates::HeaderMatching(QNetworkRequest::UserAgentHeader,
                                                               QRegularExpression(expectedUserAgent)))
            .reply();
    }
    reply->withBody(QString{"test-content"}.toUtf8());

    // Create request
    NetworkRequest request = buildRequest(requestedURL).setManager(&manager)
                                 .setMaxRedirects(maxRedirects).build();

    bool didSucceed = false, didError = false;
    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&didSucceed](const QByteArray&) {
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });

    request.fetch();
    QTest::qWait(300);

    QTEST_ASSERT(didError || didSucceed);
    // Ensures that predicates match - i.e., the header was set correctly
    QCOMPARE(didSucceed, !expectError);
    QCOMPARE(didError, expectError);
    if(didSucceed) {
        QCOMPARE(manager.matchedRequests().length(), numRedirects + 1);
        QCOMPARE(request.URL(), requestedURL);
    }
}

void TestNetworkRequest::testNetworkRequestRedirects_data()
{
    QTest::addColumn<int>("numRedirects");
    QTest::addColumn<int>("maxRedirects");

    QTest::newRow("fewer redirects than allowed (0)") << 0 << 5;
    QTest::newRow("fewer redirects than allowed (1)") << 1 << 5;
    QTest::newRow("fewer redirects than allowed (2)") << 2 << 5;
    QTest::newRow("more redirects than allowed (1, 0)") << 1 << 0;
    QTest::newRow("more redirects than allowed (2, 1)") << 2 << 1;
    QTest::newRow("more redirects than allowed (3, 2)") << 3 << 2;
}

void TestNetworkRequest::testNetworkRequestTimeoutWithRedirects()
{
    // Test that the timeout parameter is respected even when redirects are involved:
    // - Set up a request that will redirect 2 times
    // - Each request should have a delay of 250ms
    // - The timeout should be 400ms
    // -> The request should fail
    const int numRedirects = 3;
    const auto delayPerRequest = std::chrono::milliseconds{250};
    const auto timeout = std::chrono::milliseconds{400};
    const auto requestedURL = QUrl("https://example.com");

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    QStringList requestedUrls;

    auto* reply = &manager.whenGet(requestedURL).reply();

    std::vector<std::unique_ptr<QTimer>> timers;
    auto nextDelay = delayPerRequest;
    for(int i = 0; i < numRedirects; ++i) {
        auto redirectTarget = QUrl("https://example.com/redirect" + QString::number(i));

        auto timer(std::make_unique<QTimer>());
        timer->setSingleShot(true);
        timer->start(delayPerRequest);
        nextDelay += delayPerRequest;

        reply->withRedirect(redirectTarget).withFinishDelayUntil(timer.get(), &QTimer::timeout);
        reply = &manager.whenGet(redirectTarget).reply();

        timers.push_back(std::move(timer));
    }
    reply->withBody(QString{"test-content"}.toUtf8());

    // Create request
    NetworkRequest request = buildRequest(requestedURL).setManager(&manager)
                                 .setTimeout(timeout)
                                 .setMaxRedirects(NetworkRequest::UNLIMITED_REDIRECTS).build();

    bool didSucceed = false, didError = false;
    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&didSucceed](const QByteArray&) {
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });

    request.fetch();
    // Wait until the timeout should have occured
    QTest::qWait(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count() + TIMEOUT_GRACE_MS);

    QTEST_ASSERT(didError || didSucceed);
    QCOMPARE(didSucceed, false);
    QCOMPARE(didError, true);
}

void TestNetworkRequest::testNetworkRequestSecurityParameter()
{
    // Test that requests with allowInsecure() set to false fail when the URL uses an insecure schema
    QFETCH(QUrl, targetURL);
    QFETCH(bool, allowInsecure);
    QFETCH(bool, shouldSucceed);

    // Create mock reply
    // Create and configure the mocked network access manager
    MockNetworkAccess::Manager<QNetworkAccessManager> manager;

    QStringList requestedUrls;

    auto* reply = &manager.whenGet(targetURL).reply();
    reply->withBody(QString{"test-content"}.toUtf8());

    // Create request
    NetworkRequest request = buildRequest(targetURL).setManager(&manager)
                                 .setAllowInsecure(allowInsecure).build();

    bool didSucceed = false, didError = false;
    // Check request
    QSignalSpy spy(&request, &NetworkRequest::success);
    connect(&request, &NetworkRequest::success, [&didSucceed](const QByteArray&) {
        didSucceed = true;
    });

    QSignalSpy errorSpy(&request, &NetworkRequest::failure);
    connect(&request, &NetworkRequest::failure, [&didError]() { didError = true; });

    request.fetch();
    QTest::qWait(300);

    QTEST_ASSERT(didError || didSucceed);
    QCOMPARE(didSucceed, shouldSucceed);
    QCOMPARE(didError, !shouldSucceed);
}

void TestNetworkRequest::testNetworkRequestSecurityParameter_data()
{
    QTest::addColumn<QUrl>("targetURL");
    QTest::addColumn<bool>("allowInsecure");
    QTest::addColumn<bool>("shouldSucceed");

    QTest::newRow("secure protocol with allowInsecure=false succeeds") << QUrl("https://example.com") << false << true;
    QTest::newRow("secure protocol with allowInsecure=true succeeds") << QUrl("https://example.com") << true << true;
    QTest::newRow("insecure protocol with allowInsecure=false fails") << QUrl("http://example.com") << false << false;
    QTest::newRow("insecure protocol with allowInsecure=true succeeds") << QUrl("http://example.com") << true << true;
}
