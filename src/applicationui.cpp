/*
 * Copyright (c) 2011-2013 BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "applicationui.hpp"
#include "Talk2WatchInterface.h"
#include "UdpModule.h"

#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include <bb/cascades/LocaleHandler>
#include <bb/ApplicationInfo>

#include <QDateTime>

using namespace bb::cascades;

ApplicationUI::ApplicationUI(bb::cascades::Application *app) :
        QObject(app)
{
    // prepare the localization
    m_pTranslator = new QTranslator(this);
    m_pLocaleHandler = new LocaleHandler(this);

    m_model = new GroupDataModel(this);
    m_model->setGrouping(ItemGrouping::None);

    saved = true;

    bool res = QObject::connect(m_pLocaleHandler, SIGNAL(systemLanguageChanged()), this, SLOT(onSystemLanguageChanged()));
    // This is only available in Debug builds
    Q_ASSERT(res);
    // Since the variable is not used in the app, this is added to avoid a
    // compiler warning
    Q_UNUSED(res);

    // initial load
    onSystemLanguageChanged();

    // Create scene document from main.qml asset, the parent is set
    // to ensure the document gets destroyed properly at shut down.
    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);
    qml->setContextProperty("_app", this);

    // Create root object for the UI
    AbstractPane *root = qml->createRootObject<AbstractPane>();

    // Set created root object as the application scene
    app->setScene(root);

//    m_label = root->findChild<Label*>("m_label");
    m_listView = root->findChild<ListView*>("m_listView");

//    m_label->setText("Use ScriptMode to start GPS");

    bb::ApplicationInfo appInfo;
    QString version = appInfo.version() ;

	settings.setValue("appName", "GPS");
	settings.setValue("version", version);
	settings.setValue("appKey", "614bf149-5f54-4c41-8df0-4ec1ef94cea2"); // generated randomly online

    t2w = new Talk2WatchInterface(this);
    connect(t2w, SIGNAL(transmissionReady()), this, SLOT(onTransmissionReady()));

    udp = new UdpModule;
    udp->listenOnPort(9111);
    connect(udp, SIGNAL(reveivedData(QString)), this, SLOT(onUdpDataReceived(QString)));

	source = QGeoPositionInfoSource::createDefaultSource(this);
}

void ApplicationUI::onSystemLanguageChanged()
{
    QCoreApplication::instance()->removeTranslator(m_pTranslator);
    // Initiate, load and install the application translation files.
    QString locale_string = QLocale().name();
    QString file_name = QString("GPS_%1").arg(locale_string);
    if (m_pTranslator->load(file_name, "app/native/qm")) {
        QCoreApplication::instance()->installTranslator(m_pTranslator);
    }
}

void ApplicationUI::onTransmissionReady()
{
	// Authorize Watch2Watch to send action to Talk2Watch at startup. This is not necessary usually, I do it to verify the state of T2W API.
	// If it's working, Settings page give access to T2W action creation.
	authorizeAppWithT2w();
}

void ApplicationUI::authorizeAppWithT2w()
{
	// T2W authorization request --- Limited to Pro version as of 2014-02-09
	if (t2w->isTalk2WatchProInstalled() || t2w->isTalk2WatchProServiceInstalled()) {
		QString t2wAuthUdpPort = "9111"; // Random port, use the same value as the port in udp->listenOnPort(xxxx);
		QString description = "This app is a helper app for Brandon, use all the code you want freely!";
		t2w->setAppValues(settings.value("appName").toString(), settings.value("version").toString(), settings.value("appKey").toString(), "UDP", t2wAuthUdpPort, description);
		t2w->sendAppAuthorizationRequest();
	}
}

void ApplicationUI::onUdpDataReceived(QString _data)
{
	qDebug() << "onUdpDataReceived in..." << _data;
	if(_data=="AUTH_SUCCESS") {
		qDebug() << "Auth_Success!!!";
		// Create action as soon as this app has been aknowledge by T2W. You can call this
		// at every startup (like this app does), but it's not ideal. Change your code accordingly.
		t2wActionTitle << "Save location";
		t2wActionCommand << "GPSapp_SAVE_LOCATION";
		t2wActionDescription << "Save this location to Pinguin";
		if (t2wActionTitle.size() > 0)
			t2w->createAction(t2wActionTitle[0], t2wActionCommand[0], t2wActionDescription[0]);
		return;
	}
	if (_data=="CREATE_ACTION_SUCCESS") {
		qDebug() << "Create_Action_success";
		// Only create next action after the first one has been acked
		if (t2wActionTitle.size() < 1)
			return;
		t2wActionTitle.removeFirst();
		t2wActionCommand.removeFirst();
		t2wActionDescription.removeFirst();
		if (t2wActionTitle.size() > 0)
			t2w->createAction(t2wActionTitle[0], t2wActionCommand[0], t2wActionDescription[0]);
	}
	if (_data=="GPSapp_SAVE_LOCATION") {
		qDebug() << "ScriptMode sent a call to GPSapp_SAVE_LOCATION";
//		m_label->setText(m_label->text() + "\n\nGPS started...");
		if (source) {
			// saved value is there to save only once the location
			saved = false;
			source->setProperty( "canRunInBackground", true );
			source->startUpdates();
			connect(source, SIGNAL(positionUpdated(const QGeoPositionInfo &)), this, SLOT(positionUpdated(const QGeoPositionInfo &)));
		}
	}
}

void ApplicationUI::positionUpdated(const QGeoPositionInfo & pos)
{
	double accuracy = source->lastKnownPosition().attribute(QGeoPositionInfo::HorizontalAccuracy);
	QString lat = QString::number(source->lastKnownPosition().coordinate().latitude());
	QString lon = QString::number(source->lastKnownPosition().coordinate().longitude());
	qDebug() << accuracy;
	qDebug() << lat << lon;
//	m_label->setText(m_label->text() + "\n" + lat + ", " + lon + "  /// accuracy: " + QString::number(accuracy) + " meters");
	if (accuracy < 100) {
		source->stopUpdates();
		qDebug() << "GPS Stopped";
		if (!saved) {
			//		m_label->setText(m_label->text() + "\n\nGPS stopped... Sending to watch");
			QVariantMap entry;
			entry["time"] = QDateTime::currentDateTime().toString("hh:mm:ss");
			entry["latLON"] = lat + ", " + lon;
			entry["dateOfDay"] = QDateTime::currentDateTime().toString("M/d/yy");
			m_model->insert(entry);
			t2w->sendSms("Location saved to Pinguin", lat + ", " + lon + "\nAccuracy : " + QString::number(accuracy) + " meters");
			saved = true;
		}
	}
}

bb::cascades::GroupDataModel* ApplicationUI::model() const
{
	return m_model;
}

