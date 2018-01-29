//
//
//

#include "stdafx.h"
#include "TSND121.h"

static void dump(const unsigned char* ptr, int bytes)
{
	while (0 < bytes--) {
		TRACE(" %02X", *ptr++);
	}
	TRACE("\n");
}
static void dumpdate(const unsigned char* x)
{
	TRACE("%d/%d/%d %d:%d:%d\n", 2000+x[0],x[1],x[2], x[3],x[4],x[5]);
}

CTSND121::CTSND121()
{
	m_bError = false;
	m_ticktime = 0;
	for (int i = 0; i < 3; i++) {
		m_accelerate[i] = m_angular_velocity[i] = 0.0;
	}
}
CTSND121::~CTSND121()
{
}

double CTSND121::Accelerate(int id) const
{
	return m_accelerate[id];
}
double CTSND121::AngularVelocity(int id) const
{
	return m_angular_velocity[id];
}
unsigned long CTSND121::TickTime() const
{
	return m_ticktime;
}

static void setchecksum(unsigned char* ptr, unsigned int n)
{
	unsigned char cs = 0;
	for (unsigned int i = 1; i < n; i++) {
		cs ^= *ptr++;
	}
	*ptr = cs;
}

long CTSND121::getint32(const unsigned char* ptr) const
{
	return (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
}
long CTSND121::getint24(const unsigned char* ptr) const
{
	long val = (ptr[2] << 24) | (ptr[1] << 16) | (ptr[0] << 8);
	return val >> 8;
}
double CTSND121::accelerate(int val) const
{
	return static_cast<double>(val) / 10000.0;	// 0.1mg 単位 -> g
}
double CTSND121::angularveocity(int val) const
{
	return static_cast<double>(val) / 100.0;	// 0.01dps 単位 -> dps
}
double CTSND121::accelerate(const unsigned char* ptr) const
{
	return accelerate(getint24(ptr));
}
double CTSND121::angularveocity(const unsigned char* ptr) const
{
	return angularveocity(getint24(ptr));
}

bool CTSND121::Start(LPCTSTR portname, bool(*handler)(void*), void* parameter)
{
	m_bError = false;
	if (m_port.IsConnected()) {
		m_port.Close();
		Sleep(200);
	}
	m_logger.parameter = parameter;
	m_logger.handler = handler;

	m_rxptr = m_rxbuffer;
	m_port.ReceiveHandler(Receive, this);
	if (!m_port.Open(portname, 115200, CSerialPort::N81)) {
		return false;
	}
	// 加速度・角速度パラメータ設定
	unsigned char initcmd[] = {
		0x9a,	// header
		0x16,	// cmd
		0x01,	// data
		0x0a,	// data1
		0x00,	// data2
		0x00,	// checksum
	};
	setchecksum(initcmd, sizeof(initcmd));
	m_port.Write(initcmd, sizeof(initcmd));
	// 計測開始
	unsigned char startcmd[] = {
		0x9a,	// header
		0x13,	// cmd
		0x00,	// smode
		0x00,	// syear
		0x01,	// smonth
		0x01,	// sday
		0x00,	// shour
		0x00,	// smin
		0x00,	// ssec
		0x00,	// emode
		0x00,	// eyear
		0x01,	// emonth
		0x01,	// eday
		0x00,	// ehour
		0x00,	// emin
		0x00,	// esec
		0x00,	// checksum
	};
	setchecksum(startcmd, sizeof(startcmd));
	m_port.Write(startcmd, sizeof(startcmd));
	return true;
}

void CTSND121::Stop()
{
	m_port.Close();
}

void CTSND121::Transmit()
{
}
void CTSND121::Transmit(void* ptr)
{
	static_cast<CTSND121*>(ptr)->Transmit();
}

void CTSND121::Receive(int c)
{
	if (m_rxptr == m_rxbuffer) {
		// 0x9A を待ちます
		if (c == 0x9a) {
			*m_rxptr++ = c;
			m_parameterbytes = 0;
			m_checksum = c;
		}
//else { TRACE("!%02X\n", c); }
		return;
	} else if (&m_rxbuffer[sizeof(m_rxbuffer)] <= m_rxptr) {
		// バッファオーバーラン
		m_bError = true;
		m_rxptr = m_rxbuffer;
		return;
	}
	*m_rxptr++ = c;
	m_checksum ^= c;
	if (m_rxptr == &m_rxbuffer[2]) {
//TRACE(":%02X\n",c);
		// パラメータの受信準備
		switch (c) {
		// イベント
		case 0x80:	// 加速度角速度計測データ通知
			m_parameterbytes = 22;
			break;
		case 0x88:	// 計測開始通知
			m_parameterbytes = 1;
			break;
		// レスポンス
		case 0x8f:	// コマンドレスポンス
			m_parameterbytes = 1;
			break;
		case 0x93:	// 計測時刻応答
			m_parameterbytes = 13;
			break;
/*
			// イベント
			case 0x80:	// 加速度角速度計測データ通知
			case 0x81:	// 地磁気計測データ通知
			case 0x82:	// 気圧計測データ通知
			case 0x83:	// バッテリ電圧データ通知
			case 0x84:	// 外部拡張端子データ通知
			case 0x85:	// 外部拡張端子エッジ検出通知
			case 0x86:	// 外部拡張I2C受信データ通知
			case 0x87:	// 計測エラー通知
			case 0x88:	// 計測開始通知
			case 0x89:	// 計測終了通知
			// レスポンス
			case 0x8f:	// コマンドレスポンス
			case 0x90:	// 機器情報取得応答
			case 0x92:	// 時刻取得応答
			case 0x93:	// 計測時刻応答
			case 0x97:	// 加速/角速度計測設定応答
			case 0x99:	// 地磁気計測設定応答
			case 0x9b:	// 気圧計測設定応答
			case 0x9d:	// バッテリ電圧計測設定応答
			case 0x9f:	// 外部拡張端子計測&エッジデータ出力設定応答
			case 0xa1:	// 外部拡張I2C通信設定応答
			case 0xa3:	// 加速度センサ計測レンジ設定応答
			case 0xa6:	// 角速度センサ計測レンジ設定応答
			case 0xaa:	// 外部拡張I2C通信デバイス設定応答
			case 0xab:	// 外部拡張I2C通信テスト応答
			case 0xad:	// オプションボタン操作モード設定応答
			case 0xaf:	// 計測記録上書き設定応答
			case 0xb1:	// 外部拡張端子設定応答
			case 0xb3:	// ブザー音量設定応答
			case 0xb6:	// 計測データ記録エントリ件数応答
			case 0xb7:	// 計測データ記録エントリ応答
			case 0xb8:	// 計測データ記録エントリ詳細応答
			case 0xb9:	// 計測データ記録メモリ読み出し完了応答
			case 0xba:	// 計測データ記録メモリ残容量応答
			case 0xbb:	// バッテリ状態応答
			case 0xbc:	// 動作状態応答
			case 0xbd:	// 加速度センサオフセット値応答
			case 0xbe:	// 角速度センサオフセット値応答
			case 0xd1:	// オートパワーオフ時間設定取得応答
			case 0xd3:	// オフライン計測Bluetooth 接続受付設定取得応答
*/
		default:
			ASSERT(FALSE);
		}
		return;
	}
	if (0 < m_parameterbytes--) {
		// パラメーターを受信します
		return;
	}
	m_rxptr = m_rxbuffer;
	// チェックサムを確認します
	if (m_checksum != 0) {
		m_bError = true;	// 通信エラー
		return;
	}
	// 応答処理
	switch (m_rxbuffer[1]) {
	// イベント
	case 0x80:	// 加速度角速度計測データ通知
		m_ticktime = getint32(&m_rxbuffer[2]);
		// 24bit を 32bit に変換します
		for (int i = 0; i < sizeof(m_rawdata.data) / sizeof(m_rawdata.data[0]); i++) {
			m_rawdata.data[i] = getint24(&m_rxbuffer[2 + 4 + 3 * i]);
		}
		// 加速度を正規化して格納します
		for (int i = 0; i < sizeof(m_accelerate) / sizeof(m_accelerate[0]); i++) {
			m_accelerate[i] = accelerate(m_rawdata.accelerate[i]);
		}
		// 角速度を正規化して格納します
		for (int i = 0; i < sizeof(m_angular_velocity) / sizeof(m_angular_velocity[0]); i++) {
			m_angular_velocity[i] = accelerate(m_rawdata.angular[i]);
		}
//TRACE("%f %f %f\n", m_accelerate[0], m_accelerate[1], m_accelerate[2]);
		if (m_logger.handler != NULL) {
			// LOG 生成を呼び出します
			if (!m_logger.handler(m_logger.parameter)) {
				m_logger.handler = NULL;
			}
		}
		break;
	case 0x88:	// 計測開始通知
//dump(&m_rxbuffer[2], 2);
		break;
	// レスポンス
	case 0x8f:	// コマンドレスポンス
		if (m_rxbuffer[2] != 0) {
			m_bError = true;	// コマンドエラー
		}
		break;
	case 0x93:	// 計測時刻応答
//TRACE("%d\n", m_rxbuffer[2]); dumpdate(&m_rxbuffer[3]); dumpdate(&m_rxbuffer[9]);
		break;
/*
	// イベント
	case 0x80:	// 加速度角速度計測データ通知
	case 0x81:	// 地磁気計測データ通知
	case 0x82:	// 気圧計測データ通知
	case 0x83:	// バッテリ電圧データ通知
	case 0x84:	// 外部拡張端子データ通知
	case 0x85:	// 外部拡張端子エッジ検出通知
	case 0x86:	// 外部拡張I2C受信データ通知
	case 0x87:	// 計測エラー通知
	case 0x88:	// 計測開始通知
	case 0x89:	// 計測終了通知
	// レスポンス
	case 0x8f:	// コマンドレスポンス
	case 0x90:	// 機器情報取得応答
	case 0x92:	// 時刻取得応答
	case 0x93:	// 計測時刻応答
	case 0x97:	// 加速/角速度計測設定応答
	case 0x99:	// 地磁気計測設定応答
	case 0x9b:	// 気圧計測設定応答
	case 0x9d:	// バッテリ電圧計測設定応答
	case 0x9f:	// 外部拡張端子計測&エッジデータ出力設定応答
	case 0xa1:	// 外部拡張I2C通信設定応答
	case 0xa3:	// 加速度センサ計測レンジ設定応答
	case 0xa6:	// 角速度センサ計測レンジ設定応答
	case 0xaa:	// 外部拡張I2C通信デバイス設定応答
	case 0xab:	// 外部拡張I2C通信テスト応答
	case 0xad:	// オプションボタン操作モード設定応答
	case 0xaf:	// 計測記録上書き設定応答
	case 0xb1:	// 外部拡張端子設定応答
	case 0xb3:	// ブザー音量設定応答
	case 0xb6:	// 計測データ記録エントリ件数応答
	case 0xb7:	// 計測データ記録エントリ応答
	case 0xb8:	// 計測データ記録エントリ詳細応答
	case 0xb9:	// 計測データ記録メモリ読み出し完了応答
	case 0xba:	// 計測データ記録メモリ残容量応答
	case 0xbb:	// バッテリ状態応答
	case 0xbc:	// 動作状態応答
	case 0xbd:	// 加速度センサオフセット値応答
	case 0xbe:	// 角速度センサオフセット値応答
	case 0xd1:	// オートパワーオフ時間設定取得応答
	case 0xd3:	// オフライン計測Bluetooth 接続受付設定取得応答
*/
	default:
		break;
	}
}
void CTSND121::Receive(void* parameter, const void* buffer, unsigned int bytes)
{
	const unsigned char* ptr = static_cast<const unsigned char*>(buffer);
	while (0 < bytes--) {
		static_cast<CTSND121*>(parameter)->Receive(*ptr++);
	}
}
/*
9A 8F 00 15
9A 93 01 0C 01 14 09 22 34 64 01 01 00 00 00 6A
9A 88 00 12
9A 80 5D 51 0E 02 D3 F5 FF 09 F6 FF BF 23 00 C6 00 00 0C 00 00 BB FF FF 2E
*/