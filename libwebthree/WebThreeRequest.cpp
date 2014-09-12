/*
 This file is part of cpp-ethereum.
 
 cpp-ethereum is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 cpp-ethereum is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
 */
/** @file WebThreeRequest.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 */

#include "WebThreeRequest.h"
#include "WebThreeResponse.h"

using namespace dev;

WebThreeRequest::WebThreeRequest(WebThreeClient *_c, RLP const& _request): WebThreeMessage(_c->nextSequence(), _request)
{
	_c->send(this);
}

void WebThreeRequest::respond(WebThreeConnection *_s, RLP const& _response)
{
	WebThreeResponse(_s, this, _response);
}

