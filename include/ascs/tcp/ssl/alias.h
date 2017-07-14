/*
* alias.h
*
*  Created on: 2017-7-14
*      Author: youngwolf
*		email: mail2tao@163.com
*		QQ: 676218192
*		Community on QQ: 198941541
*
* some alias, they are deprecated.
*/

#ifndef _ASCS_SSL_ALIAS_H_
#define _ASCS_SSL_ALIAS_H_

#include "ssl.h"

namespace ascs { namespace ssl {

#ifdef ASCS_HAS_TEMPLATE_USING
template <typename Packer, typename Unpacker, typename Socket = asio::ssl::stream<asio::ip::tcp::socket>,
	template<typename, typename> class InQueue = ASCS_INPUT_QUEUE, template<typename> class InContainer = ASCS_INPUT_CONTAINER,
	template<typename, typename> class OutQueue = ASCS_OUTPUT_QUEUE, template<typename> class OutContainer = ASCS_OUTPUT_CONTAINER>
using connector_base = tcp::client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer>;
template<typename Socket, typename Pool = object_pool<Socket>> using client_base = multi_client_base<Socket, Pool>;
#else
template <typename Packer, typename Unpacker, typename Socket = asio::ssl::stream<asio::ip::tcp::socket>,
	template<typename, typename> class InQueue = ASCS_INPUT_QUEUE, template<typename> class InContainer = ASCS_INPUT_CONTAINER,
	template<typename, typename> class OutQueue = ASCS_OUTPUT_QUEUE, template<typename> class OutContainer = ASCS_OUTPUT_CONTAINER>
class connector_base : public tcp::client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer>
{
private:
	typedef client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer> super;

public:
	connector_base(asio::io_service& io_service_, asio::ssl::context& ctx) : super(io_service_, ctx) {}
};
template<typename Socket, typename Pool = object_pool<Socket>> class client_base : public multi_client_base<Socket, Pool>
{
public:
	client_base(service_pump& service_pump_, asio::ssl::context::method m) : multi_client_base<Socket, Pool>(service_pump_, m) {}
};
#endif

}} //namespace

#endif /* _ASCS_SSL_ALIAS_H_ */