/*
 * ssl.h
 *
 *  Created on: 2012-3-2
 *      Author: youngwolf
 *		email: mail2tao@163.com
 *		QQ: 676218192
 *		Community on QQ: 198941541
 *
 * make ascs support asio::ssl
 */

#ifndef _ASICS_SSL_H_
#define _ASICS_SSL_H_

#include <asio/ssl.hpp>

#include "../../object_pool.h"
#include "../client_socket.h"
#include "../client.h"
#include "../server_socket.h"
#include "../server.h"

namespace ascs { namespace ssl {

template <typename Socket>
class socket : public Socket
{
#if defined(ASCS_REUSE_OBJECT) && !defined(ASCS_REUSE_SSL_STREAM)
	#error please define ASCS_REUSE_SSL_STREAM macro explicitly if you need asio::ssl::stream to be reusable!
#endif

public:
	template<typename Arg>
	socket(Arg& arg, asio::ssl::context& ctx) : Socket(arg, ctx), authorized_(false) {}

	virtual bool is_ready() {return authorized_ && Socket::is_ready();}
	virtual void reset() {authorized_ = false;}
	bool authorized() const {return authorized_;}

protected:
	virtual void on_recv_error(const asio::error_code& ec)
	{
		if (is_ready())
		{
			authorized_ = false;
#ifndef ASCS_REUSE_SSL_STREAM
			this->status = Socket::link_status::GRACEFUL_SHUTTING_DOWN;
			this->show_info("ssl link:", "been shut down.");
			asio::error_code ec;
			this->next_layer().shutdown(ec);

			if (ec && asio::error::eof != ec) //the endpoint who initiated a shutdown will get error eof.
				unified_out::info_out("shutdown ssl link failed (maybe intentionally because of reusing)");
#endif
		}

		Socket::on_recv_error(ec);
	}

	virtual void on_handshake(const asio::error_code& ec)
	{
		if (!ec)
			unified_out::info_out("handshake success.");
		else
			unified_out::error_out("handshake failed: %s", ec.message().data());
	}

	void handle_handshake(const asio::error_code& ec) {on_handshake(ec); if (!ec) {authorized_ = true; Socket::do_start();}}

	void shutdown_ssl(bool sync = true)
	{
		if (!is_ready())
		{
			Socket::force_shutdown();
			return;
		}

		authorized_ = false;
		this->status = Socket::link_status::GRACEFUL_SHUTTING_DOWN;

		if (!sync)
		{
			this->show_info("ssl link:", "been shutting down.");
			this->next_layer().async_shutdown(this->make_handler_error([this](const asio::error_code& ec) {
				if (ec && asio::error::eof != ec) //the endpoint who initiated a shutdown will get error eof.
					unified_out::info_out("async shutdown ssl link failed (maybe intentionally because of reusing)");
			}));
		}
		else
		{
			this->show_info("ssl link:", "been shut down.");
			asio::error_code ec;
			this->next_layer().shutdown(ec);

			if (ec && asio::error::eof != ec) //the endpoint who initiated a shutdown will get error eof.
				unified_out::info_out("shutdown ssl link failed (maybe intentionally because of reusing)");
		}
	}

protected:
	volatile bool authorized_;
};

template <typename Packer, typename Unpacker, typename Socket = asio::ssl::stream<asio::ip::tcp::socket>,
	template<typename, typename> class InQueue = ASCS_INPUT_QUEUE, template<typename> class InContainer = ASCS_INPUT_CONTAINER,
	template<typename, typename> class OutQueue = ASCS_OUTPUT_QUEUE, template<typename> class OutContainer = ASCS_OUTPUT_CONTAINER>
class client_socket_base : public socket<tcp::client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer>>
{
private:
	typedef socket<tcp::client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer>> super;

public:
	client_socket_base(asio::io_service& io_service_, asio::ssl::context& ctx) : super(io_service_, ctx) {}

	void disconnect(bool reconnect = false) {force_shutdown(reconnect);}
#ifdef ASCS_REUSE_SSL_STREAM
	void force_shutdown(bool reconnect = false) {this->authorized_ = false; super::force_shutdown(reconnect);}
	void graceful_shutdown(bool reconnect = false, bool sync = true) {this->authorized_ = false; super::graceful_shutdown(reconnect, sync);}
#else
	void force_shutdown(bool reconnect = false) {graceful_shutdown(reconnect);}
	void graceful_shutdown(bool reconnect = false, bool sync = true)
	{
		if (reconnect)
			unified_out::error_out("reconnecting mechanism is not available, please define macro ASCS_REUSE_SSL_STREAM");

		this->need_reconnect = false; //ignore reconnect parameter
		this->shutdown_ssl(sync);
	}
#endif

protected:
	virtual bool do_start() //add handshake
	{
		if (!this->is_connected())
			super::do_start();
		else if (!this->authorized())
			this->next_layer().async_handshake(asio::ssl::stream_base::client, this->make_handler_error([this](const asio::error_code& ec) {this->handle_handshake(ec);}));

		return true;
	}

#ifndef ASCS_REUSE_SSL_STREAM
	virtual int prepare_reconnect(const asio::error_code& ec) {return -1;}
	virtual void on_recv_error(const asio::error_code& ec) {this->need_reconnect = false; super::on_recv_error(ec);}
#endif
	virtual void on_unpack_error() {unified_out::info_out("can not unpack msg."); force_shutdown();}

private:
	void handle_handshake(const asio::error_code& ec) {super::handle_handshake(ec); if (ec) force_shutdown();}
};

template<typename Object>
class object_pool : public ascs::object_pool<Object>
{
private:
	typedef ascs::object_pool<Object> super;

public:
	object_pool(service_pump& service_pump_, const asio::ssl::context::method& m) : super(service_pump_), ctx(m) {}
	asio::ssl::context& context() {return ctx;}

	typename object_pool::object_type create_object() {return create_object(this->sp);}
	template<typename Arg>
	typename object_pool::object_type create_object(Arg& arg) {return super::create_object(arg, ctx);}

protected:
	asio::ssl::context ctx;
};

template<typename Packer, typename Unpacker, typename Server = i_server, typename Socket = asio::ssl::stream<asio::ip::tcp::socket>,
	template<typename, typename> class InQueue = ASCS_INPUT_QUEUE, template<typename> class InContainer = ASCS_INPUT_CONTAINER,
	template<typename, typename> class OutQueue = ASCS_OUTPUT_QUEUE, template<typename> class OutContainer = ASCS_OUTPUT_CONTAINER>
class server_socket_base : public socket<tcp::server_socket_base<Packer, Unpacker, Server, Socket, InQueue, InContainer, OutQueue, OutContainer>>
{
private:
	typedef socket<tcp::server_socket_base<Packer, Unpacker, Server, Socket, InQueue, InContainer, OutQueue, OutContainer>> super;

public:
	server_socket_base(Server& server_, asio::ssl::context& ctx) : super(server_, ctx) {}

	void disconnect() {force_shutdown();}
#ifdef ASCS_REUSE_SSL_STREAM
	void force_shutdown() {this->authorized_ = false; super::force_shutdown();}
	void graceful_shutdown(bool sync = false) {this->authorized_ = false; super::graceful_shutdown(sync);}
#else
	void force_shutdown() {graceful_shutdown();} //must with async mode (the default value), because server_base::uninit will call this function
	void graceful_shutdown(bool sync = false) {this->shutdown_ssl(sync);}
#endif

protected:
	virtual bool do_start() //add handshake
	{
		if (!this->authorized())
			this->next_layer().async_handshake(asio::ssl::stream_base::server, this->make_handler_error([this](const asio::error_code& ec) {this->handle_handshake(ec);}));

		return true;
	}

	virtual void on_unpack_error() {unified_out::info_out("can not unpack msg."); force_shutdown();}

private:
	void handle_handshake(const asio::error_code& ec) {super::handle_handshake(ec); if (ec) this->server.del_socket(this->shared_from_this());}
};

#ifdef ASCS_HAS_TEMPLATE_USING
template <typename Packer, typename Unpacker, typename Socket = asio::ssl::stream<asio::ip::tcp::socket>,
	template<typename, typename> class InQueue = ASCS_INPUT_QUEUE, template<typename> class InContainer = ASCS_INPUT_CONTAINER,
	template<typename, typename> class OutQueue = ASCS_OUTPUT_QUEUE, template<typename> class OutContainer = ASCS_OUTPUT_CONTAINER>
using connector_base = tcp::client_socket_base<Packer, Unpacker, Socket, InQueue, InContainer, OutQueue, OutContainer>;
template<typename Socket, typename Pool = object_pool<Socket>, typename Server = i_server> using server_base = tcp::server_base<Socket, Pool, Server>;
template<typename Socket> using single_client_base = tcp::single_client_base<Socket>;
template<typename Socket, typename Pool = object_pool<Socket>> using multi_client_base = tcp::multi_client_base<Socket, Pool>;
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
template<typename Socket, typename Pool = object_pool<Socket>, typename Server = i_server> class server_base : public tcp::server_base<Socket, Pool, Server>
{
public:
	server_base(service_pump& service_pump_, asio::ssl::context::method m) : tcp::server_base<Socket, Pool, Server>(service_pump_, m) {}
};
template<typename Socket> class single_client_base : public tcp::single_client_base<Socket>
{
public:
	single_client_base(service_pump& service_pump_, asio::ssl::context& ctx) : tcp::single_client_base<Socket>(service_pump_, ctx) {}
};
template<typename Socket, typename Pool = object_pool<Socket>> class multi_client_base : public tcp::multi_client_base<Socket, Pool>
{
public:
	multi_client_base(service_pump& service_pump_, asio::ssl::context::method m) : tcp::multi_client_base<Socket, Pool>(service_pump_, m) {}
};
template<typename Socket, typename Pool = object_pool<Socket>> class client_base : public multi_client_base<Socket, Pool>
{
public:
	client_base(service_pump& service_pump_, asio::ssl::context::method m) : multi_client_base<Socket, Pool>(service_pump_, m) {}
};
#endif

}} //namespace

#endif /* _ASICS_SSL_H_ */
