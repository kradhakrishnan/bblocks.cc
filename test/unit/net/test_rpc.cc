#include <iostream>

#include "test/unit-test.h"
#include "net/rpc-data.h"

using namespace std;
using namespace dh_core;

static LogPath _log("/rpctest");

// ......................................................... test_rpcpacket ....

struct TestPacket : RPCPacket
{
	static const uint8_t OPCODE = 0xdd;

	TestPacket(const uint32_t data)
		: RPCPacket(OPCODE), data_(data)
	{}

	virtual void Encode(IOBuffer & buf)
	{
		INVARIANT(buf.Size() >= Size());

		size_t pos = 0;

		RPCPacket::Encode(buf, pos);
		data_.Encode(buf, pos);
		EncodePacketHash(buf);
	}

	virtual void Decode(IOBuffer & buf)
	{
		INVARIANT(buf.Size() >= Size());

		size_t pos = 0;

		RPCPacket::Decode(buf, pos);
		data_.Decode(buf, pos);
		INVARIANT(IsPacketValid(buf));
	}

	virtual size_t Size() const
	{
		return RPCPacket::Size()
			+ data_.Size();
	}

	UInt32 data_;
};

static void
test_rpcpacket()
{
	TestPacket data(9);

	IOBuffer && buf = IOBuffer::Alloc(data.Size());

	data.Encode(buf);

	INFO(_log) << "opcode : " << (int) data.opcode_.Get()
		   << " opver : " << (int) data.opver_.Get()
		   << " size : " << data.size_.Get()
		   << " cksum : " << data.cksum_.Get();

	data.Decode(buf);
}

// ......................................................... test_datatypes ....

struct Data : RPCData
{
	virtual void Encode(IOBuffer & buf, size_t & pos)
	{
		i16_.Encode(buf, pos);
		i32_.Encode(buf, pos);
		i64_.Encode(buf, pos);
		str_.Encode(buf, pos);
		lu32_.Encode(buf, pos);
		lu64_.Encode(buf, pos);
		lstr_.Encode(buf, pos);
		raw_.Encode(buf, pos);
	}

	virtual void Decode(IOBuffer & buf, size_t & pos)
	{
		i16_.Decode(buf, pos);
		i32_.Decode(buf, pos);
		i64_.Decode(buf, pos);
		str_.Decode(buf, pos);
		lu32_.Decode(buf, pos);
		lu64_.Decode(buf, pos);
		lstr_.Decode(buf, pos);
		raw_.Decode(buf, pos);
	}

	virtual size_t Size() const
	{
		return sizeof(i16_)
			+ sizeof(i32_)
			+ sizeof(i64_)
			+ str_.Size()
			+ lu32_.Size()
			+ lu64_.Size()
			+ lstr_.Size();
			+ raw_.Size();
	}

	UInt16 i16_;
	UInt32 i32_;
	UInt64 i64_;
	String str_;
	List<UInt32> lu32_;
	List<UInt64> lu64_;
	List<String> lstr_;
	Raw<10> raw_;
};

static void
test_datatypes()
{
	Data data;
	size_t pos;

	List<UInt32> lu32({ UInt32(2), UInt32(4), UInt32(16) });
	List<UInt64> lu64({ UInt64(32), UInt64(64), UInt64(128) });
	List<String> lstr({ String("a"), String("b"), String("c") });
	uint8_t rawdata[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	Raw<10> raw(rawdata);

	data.i16_.Set(257);
	data.i32_.Set(55);
	data.i64_.Set(555);
	data.str_.Set("5555");
	data.lu32_.Set(lu32);
	data.lu64_.Set(lu64);
	data.lstr_.Set(lstr);
	data.raw_.Set(rawdata);

	INFO(_log) << "Encoding. size=" << data.Size();

	IOBuffer buf = IOBuffer::Alloc(data.Size());
	pos = 0;
	data.Encode(buf, pos);

	INFO(_log) << "Decoding";

	Data data2;
	pos = 0;
	data2.Decode(buf, pos);

	INFO(_log) << "Checking data";

	INVARIANT(data.i16_ == 257);
	INVARIANT(data.i32_ == 55);
	INVARIANT(data.i64_ == 555);
	INVARIANT(data.str_ == "5555");
	INVARIANT(data.lu32_ == lu32);
	INVARIANT(data.lu64_ == lu64);
	INVARIANT(data.lstr_ == lstr);
	INVARIANT(data.raw_ == rawdata);
}

//.................................................................... main ....

int
main(int argc, char ** argv)
{
	srand(time(NULL));

	InitTestSetup();

	TEST(test_datatypes);
	TEST(test_rpcpacket);

	TeardownTestSetup();

	return 0;
}