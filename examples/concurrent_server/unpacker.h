#ifndef UNPACKER_H_
#define UNPACKER_H_

#define MSG_MAX_LEN		(4 + 512)

#include <ascs/ext/ext.h>
using namespace ascs::ext;

//head (4 bytes, readable) + body, not stripped, cannot exceed MSG_MAX_LEN
class echo_unpacker : public ascs::tcp::i_unpacker<basic_buffer>
{
public:
	echo_unpacker() {raw_buff.assign(MSG_MAX_LEN); raw_buff.size(0);}

public:
	virtual void reset() {raw_buff.size(0);}
	virtual bool parse_msg(size_t bytes_transferred, container_type& msg_can)
	{
		auto ok = bytes_transferred == raw_buff.size();
		if (!ok && raw_buff.empty() && bytes_transferred == raw_buff.buffer_size())
		{
			int len;
			sscanf(raw_buff.data(), "%04d", &len);
			if ((size_t) (len + 4) == raw_buff.buffer_size())
			{
				ok = true;
				raw_buff.size(bytes_transferred);
			}
		}

		if (ok)
		{
			msg_can.emplace_back(std::move(raw_buff));
			raw_buff.assign(MSG_MAX_LEN);
			raw_buff.size(0);
		}

		return ok;
	}

	//a return value of 0 indicates that the read operation is complete. a non-zero value indicates the maximum number
	//of bytes to be read on the next call to the stream's async_read_some function. ---asio::async_read
	virtual size_t completion_condition(const asio::error_code& ec, size_t bytes_transferred)
	{
		if (!ec)
		{
			if (raw_buff.empty())
			{
				if (bytes_transferred < 4)
					return asio::detail::default_max_transfer_size;

				int len;
				sscanf(raw_buff.data(), "%04d", &len);
				assert((size_t) (len + 4) <= raw_buff.buffer_size());

				if ((size_t) (len + 4) > raw_buff.buffer_size())
				{
					assert(false);
					return 0;
				}
				else
					raw_buff.size(4 + len);
			}

			if (bytes_transferred < raw_buff.size())
				return asio::detail::default_max_transfer_size;
			else if (bytes_transferred > raw_buff.size())
				assert(false);
		}

		return 0;
	}

	//this is just to satisfy the compiler, it's not a real scatter-gather buffer,
	//if you introduce a ring buffer, then you will have the chance to provide a real scatter-gather buffer.
#ifdef ASCS_SCATTERED_RECV_BUFFER
	virtual buffer_type prepare_next_recv() {return buffer_type(1, asio::buffer(raw_buff.data(), raw_buff.buffer_size()));}
#else
	virtual buffer_type prepare_next_recv() {return asio::buffer(raw_buff.data(), raw_buff.buffer_size());}
#endif

private:
	msg_type raw_buff;
};

#endif //UNPACKER_H_
