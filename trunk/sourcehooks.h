/*
    This file is part of SourceOP.

    SourceOP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SourceOP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SOURCEHOOKS_H
#define SOURCEHOOKS_H

#include "mmsource/sourcehook/sourcehook.h"
#include "mmsource/sourcehook/sourcehook_impl.h"
extern SourceHook::ISourceHook *g_SHPtr;
//extern SourceHook::CSourceHookImpl g_SourceHook;
extern SourceHook::Plugin g_PLID;
extern SourceHook::CallClass<IServerGameDLL> *servergame_cc;
extern SourceHook::CallClass<IVEngineServer> *engine_cc;

#endif
