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

#ifndef ApplicationUI_HPP_
#define ApplicationUI_HPP_

#include <QObject>
#include <QStringList>
#include <QSettings>
//#include <bb/cascades/Label>
#include <bb/cascades/ListView>
#include <bb/cascades/GroupDataModel>

#include <QtLocationSubset/QGeoPositionInfo>
#include <QtLocationSubset/QGeoPositionInfoSource>

class Talk2WatchInterface;
class UdpModule;

using namespace QtMobilitySubset;

namespace bb
{
    namespace cascades
    {
        class Application;
        class LocaleHandler;
    }
}

class QTranslator;

/*!
 * @brief Application object
 *
 *
 */

class ApplicationUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bb::cascades::GroupDataModel *model READ model CONSTANT);

public:
    ApplicationUI(bb::cascades::Application *app);
    virtual ~ApplicationUI() { }

private slots:
    	void onSystemLanguageChanged();
        void onTransmissionReady();
        void onUdpDataReceived(QString _data);
        void positionUpdated(const QGeoPositionInfo & pos);

private:
    QTranslator* m_pTranslator;
    bb::cascades::LocaleHandler* m_pLocaleHandler;

    void authorizeAppWithT2w();
    bb::cascades::GroupDataModel* model() const;

    Talk2WatchInterface* t2w;
    UdpModule *udp;

//    bb::cascades::Label* m_label;
    bb::cascades::ListView* m_listView;
    QSettings settings;
	QStringList t2wActionTitle;
	QStringList t2wActionCommand;
	QStringList t2wActionDescription;
	bool saved;
	QGeoPositionInfoSource *source;
    bb::cascades::GroupDataModel* m_model;
};

#endif /* ApplicationUI_HPP_ */