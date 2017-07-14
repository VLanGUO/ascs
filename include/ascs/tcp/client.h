/*
 * client.h
 *
 *  Created on: 2012-3-2
 *      Author: youngwolf
 *		email: mail2tao@163.com
 *		QQ: 676218192
 *		Community on QQ: 198941541
 *
 * this class only used at client endpoint
 */

#ifndef _ASCS_CLIENT_H_
#define _ASCS_CLIENT_H_

#include "../socket_service.h"

namespace ascs { namespace tcp {

#ifdef ASCS_HAS_TEMPLATE_USING
template<typename Socket> using single_client_base = single_socket_service<Socket>;
#else
template<typename Socket> class single_client_base : public single_socket_service<Socket>
{
public:
	single_client_base(service_pump& service_pump_) : single_socket_service<Socket>(service_pump_) {}
	template<typename Arg>
	single_client_base(service_pump& service_pump_, Arg& arg) : single_socket_service<Socket>(service_pump_, arg) {}
};
#endif

template<typename Socket, typename Pool = object_pool<Socket>>
class multi_client_base : public multi_socket_service<Socket, Pool>
{
private:
	typedef multi_socket_service<Socket, Pool> super;

public:
	multi_client_base(service_pump& service_pump_) : super(service_pump_) {}
	template<typename Arg>
	multi_client_base(service_pump& service_pump_, const Arg& arg) : super(service_pump_, arg) {}

	//connected link size, may smaller than total object size (object_pool::size)
	size_t valid_size()
	{
		size_t size = 0;
		this->do_something_to_all([&size](typename Pool::object_ctype& item) {if (item->is_connected()) ++size;});
		return size;
	}

	using super::add_socket;
	typename Pool::object_type add_socket()
	{
		auto socket_ptr(this->create_object());
		return this->add_socket(socket_ptr, false) ? socket_ptr : typename Pool::object_type();
	}
	typename Pool::object_type add_socket(unsigned short port, const std::string& ip = ASCS_SERVER_IP)
	{
		auto socket_ptr(this->create_object());
		socket_ptr->set_server_addr(port, ip);
		return this->add_socket(socket_ptr, false) ? socket_ptr : typename Pool::object_type();
	}

	///////////////////////////////////////////////////
	//msg sending interface
	TCP_BROADCAST_MSG(broadcast_msg, send_msg)
	TCP_BROADCAST_MSG(broadcast_native_msg, send_native_msg)
	//guarantee send msg successfully even if can_overflow equal to false
	//success at here just means put the msg into tcp::socket_base's send buffer
	TCP_BROADCAST_MSG(safe_broadcast_msg, safe_send_msg)
	TCP_BROADCAST_MSG(safe_broadcast_native_msg, safe_send_native_msg)
	//send message with sync mode
	TCP_BROADCAST_MSG(sync_broadcast_msg, sync_send_msg)
	TCP_BROADCAST_MSG(sync_broadcast_native_msg, sync_send_native_msg)
	//msg sending interface
	///////////////////////////////////////////////////

	//functions with a socket_ptr parameter will remove the link from object pool first, then call corresponding function, if you want to reconnect to the server,
	//please call socket_ptr's 'disconnect' 'force_shutdown' or 'graceful_shutdown' with true 'reconnect' directly.
	void disconnect(typename Pool::object_ctype& socket_ptr) {this->del_object(socket_ptr); socket_ptr->disconnect();}
	void disconnect(bool reconnect = false) {this->do_something_to_all([=](typename Pool::object_ctype& item) {item->disconnect(reconnect);});}
	void force_shutdown(typename Pool::object_ctype& socket_ptr) {this->del_object(socket_ptr); socket_ptr->force_shutdown();}
	void force_shutdown(bool reconnect = false) {this->do_something_to_all([=](typename Pool::object_ctype& item) {item->force_shutdown(reconnect);});}
	void graceful_shutdown(typename Pool::object_ctype& socket_ptr, bool sync = true) {this->del_object(socket_ptr); socket_ptr->graceful_shutdown(false, sync);}
	void graceful_shutdown(bool reconnect = false, bool sync = true) {this->do_something_to_all([=](typename Pool::object_ctype& item) {item->graceful_shutdown(reconnect, sync);});}

protected:
	virtual void uninit() {this->stop(); force_shutdown();} //if you wanna graceful shutdown, call graceful_shutdown before service_pump::stop_service invocation.
};

#ifdef ASCS_HAS_TEMPLATE_USING
template<typename Socket, typename Pool = object_pool<Socket>> using client_base = multi_client_base<Socket, Pool>;
#else
template<typename Socket, typename Pool = object_pool<Socket>> class client_base : public multi_client_base<Socket, Pool>
{
public:
	client_base(service_pump& service_pump_) : multi_client_base<Socket, Pool>(service_pump_) {}
};
#endif

}} //namespace

#endif /* _ASCS_CLIENT_H_ */
