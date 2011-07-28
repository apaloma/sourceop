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

// Changes _DEBUG to _SOPDEBUG
// This is because some class structures change when _DEBUG is enabled and
// could potentially cause problems.
#ifdef _DEBUG
    #ifndef _SOPDEBUG
        #define _SOPDEBUG
    #endif
    //#undef _DEBUG
#endif
#define DBGFLAG_ASSERT
