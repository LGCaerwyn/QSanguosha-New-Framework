/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha.

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

#ifndef CLIENTPLAYER_H
#define CLIENTPLAYER_H

#include "player.h"

class CClientUser;

class ClientPlayer : public Player
{
    Q_OBJECT

public:
    ClientPlayer(CClientUser *user, QObject *parent = 0);

    void setId(uint id) { CAbstractPlayer::setId(id); }
    QString fullGeneralName() const;
    CClientUser *user() const { return m_user; }

private:
    CClientUser *m_user;
};

#endif // CLIENTPLAYER_H
