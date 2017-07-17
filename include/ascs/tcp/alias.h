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

#ifndef _ASCS_TCP_ALIAS_H_
#define _ASCS_TCP_ALIAS_H_

#include "client_socket.h"
#include "client.h"

namespace ascs { namespace tcp {

#ifdef ASCS_HAS_TEMPLATE_USING
template <typename Packer, typename Unpacker, typename Socket = asio::ip::tcp::socket,
	template<typename, typename> class InQueue = ASCS_INPUT_QUEUE, template<typename> class InContainer = ASCS_INPUT_CONTAINER,
	template<typename, typename> class OutQueue = ASCS_OUTPUT_QUEUE, template<typename> class OutContainer = ASCS_OUTPUT_CONTAINER>
using connector_base = client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer>;
template<typename Socket, typename Pool = object_pool<Socket>> using client_base = multi_client_base<Socket, Pool>;
#else
template <typename Packer, typename Unpacker, typename Socket = asio::ip::tcp::socket,
	template<typename, typename> class InQueue = ASCS_INPUT_QUEUE, template<typename> class InContainer = ASCS_INPUT_CONTAINER,
	template<typename, typename> class OutQueue = ASCS_OUTPUT_QUEUE, template<typename> class OutContainer = ASCS_OUTPUT_CONTAINER>
class connector_base : public client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer>
{
private:
	typedef client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer> super;

public:
	connector_base(asio::io_service& io_service_) : super(io_service_) {}
	template<typename Arg> connector_base(asio::io_service& io_service_, Arg& arg) : super(io_service_, arg) {}
};
template<typename Socket, typename Pool = object_pool<Socket>> class client_base : public multi_client_base<Socket, Pool>
{
public:
	client_base(service_pump& service_pump_) : multi_client_base<Socket, Pool>(service_pump_) {}
};
#endif

}} //namespace

#endif /* _ASCS_TCP_ALIAS_H_ */