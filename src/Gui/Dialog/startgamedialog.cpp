/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of Cardirector.

    This game engine is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#include "startgamedialog.h"

#include <cclient.h>

StartGameDialog::StartGameDialog(QQuickItem *parent)
    : QQuickItem(parent)
    , m_client(NULL)
{

}

void StartGameDialog::signup(const QString &screenName, const QString &avatar)
{
    m_screenName = screenName;
    m_avatar = avatar;
}

void StartGameDialog::connectToServer(const QString &server, ushort port)
{
    m_client = new CClient(qApp);
    connect(m_client, &CClient::connected, this, &StartGameDialog::onServerConnected);
    connect(m_client, &CClient::connected, this, &StartGameDialog::serverConnected);
    QHostAddress host(server);
    m_client->connectToHost(host, port);
}

void StartGameDialog::onServerConnected()
{
    m_client->signup("", "", m_screenName, m_avatar);
}

void StartGameDialog::Init()
{
    qmlRegisterType<StartGameDialog>("Sanguosha.Dialogs", 1, 0, "StartGameDialog");
}
C_INITIALIZE_CLASS(StartGameDialog)