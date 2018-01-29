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
	return static_cast<double>(val) / 10000.0;	// 0.1mg �P�� -> g
}
double CTSND121::angularveocity(int val) const
{
	return static_cast<double>(val) / 100.0;	// 0.01dps �P�� -> dps
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
	// �����x�E�p���x�p�����[�^�ݒ�
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
	// �v���J�n
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
		// 0x9A ��҂��܂�
		if (c == 0x9a) {
			*m_rxptr++ = c;
			m_parameterbytes = 0;
			m_checksum = c;
		}
//else { TRACE("!%02X\n", c); }
		return;
	} else if (&m_rxbuffer[sizeof(m_rxbuffer)] <= m_rxptr) {
		// �o�b�t�@�I�[�o�[����
		m_bError = true;
		m_rxptr = m_rxbuffer;
		return;
	}
	*m_rxptr++ = c;
	m_checksum ^= c;
	if (m_rxptr == &m_rxbuffer[2]) {
//TRACE(":%02X\n",c);
		// �p�����[�^�̎�M����
		switch (c) {
		// �C�x���g
		case 0x80:	// �����x�p���x�v���f�[�^�ʒm
			m_parameterbytes = 22;
			break;
		case 0x88:	// �v���J�n�ʒm
			m_parameterbytes = 1;
			break;
		// ���X�|���X
		case 0x8f:	// �R�}���h���X�|���X
			m_parameterbytes = 1;
			break;
		case 0x93:	// �v����������
			m_parameterbytes = 13;
			break;
/*
			// �C�x���g
			case 0x80:	// �����x�p���x�v���f�[�^�ʒm
			case 0x81:	// �n���C�v���f�[�^�ʒm
			case 0x82:	// �C���v���f�[�^�ʒm
			case 0x83:	// �o�b�e���d���f�[�^�ʒm
			case 0x84:	// �O���g���[�q�f�[�^�ʒm
			case 0x85:	// �O���g���[�q�G�b�W���o�ʒm
			case 0x86:	// �O���g��I2C��M�f�[�^�ʒm
			case 0x87:	// �v���G���[�ʒm
			case 0x88:	// �v���J�n�ʒm
			case 0x89:	// �v���I���ʒm
			// ���X�|���X
			case 0x8f:	// �R�}���h���X�|���X
			case 0x90:	// �@����擾����
			case 0x92:	// �����擾����
			case 0x93:	// �v����������
			case 0x97:	// ����/�p���x�v���ݒ艞��
			case 0x99:	// �n���C�v���ݒ艞��
			case 0x9b:	// �C���v���ݒ艞��
			case 0x9d:	// �o�b�e���d���v���ݒ艞��
			case 0x9f:	// �O���g���[�q�v��&�G�b�W�f�[�^�o�͐ݒ艞��
			case 0xa1:	// �O���g��I2C�ʐM�ݒ艞��
			case 0xa3:	// �����x�Z���T�v�������W�ݒ艞��
			case 0xa6:	// �p���x�Z���T�v�������W�ݒ艞��
			case 0xaa:	// �O���g��I2C�ʐM�f�o�C�X�ݒ艞��
			case 0xab:	// �O���g��I2C�ʐM�e�X�g����
			case 0xad:	// �I�v�V�����{�^�����샂�[�h�ݒ艞��
			case 0xaf:	// �v���L�^�㏑���ݒ艞��
			case 0xb1:	// �O���g���[�q�ݒ艞��
			case 0xb3:	// �u�U�[���ʐݒ艞��
			case 0xb6:	// �v���f�[�^�L�^�G���g����������
			case 0xb7:	// �v���f�[�^�L�^�G���g������
			case 0xb8:	// �v���f�[�^�L�^�G���g���ڍ׉���
			case 0xb9:	// �v���f�[�^�L�^�������ǂݏo����������
			case 0xba:	// �v���f�[�^�L�^�������c�e�ʉ���
			case 0xbb:	// �o�b�e����ԉ���
			case 0xbc:	// �����ԉ���
			case 0xbd:	// �����x�Z���T�I�t�Z�b�g�l����
			case 0xbe:	// �p���x�Z���T�I�t�Z�b�g�l����
			case 0xd1:	// �I�[�g�p���[�I�t���Ԑݒ�擾����
			case 0xd3:	// �I�t���C���v��Bluetooth �ڑ���t�ݒ�擾����
*/
		default:
			ASSERT(FALSE);
		}
		return;
	}
	if (0 < m_parameterbytes--) {
		// �p�����[�^�[����M���܂�
		return;
	}
	m_rxptr = m_rxbuffer;
	// �`�F�b�N�T�����m�F���܂�
	if (m_checksum != 0) {
		m_bError = true;	// �ʐM�G���[
		return;
	}
	// ��������
	switch (m_rxbuffer[1]) {
	// �C�x���g
	case 0x80:	// �����x�p���x�v���f�[�^�ʒm
		m_ticktime = getint32(&m_rxbuffer[2]);
		// 24bit �� 32bit �ɕϊ����܂�
		for (int i = 0; i < sizeof(m_rawdata.data) / sizeof(m_rawdata.data[0]); i++) {
			m_rawdata.data[i] = getint24(&m_rxbuffer[2 + 4 + 3 * i]);
		}
		// �����x�𐳋K�����Ċi�[���܂�
		for (int i = 0; i < sizeof(m_accelerate) / sizeof(m_accelerate[0]); i++) {
			m_accelerate[i] = accelerate(m_rawdata.accelerate[i]);
		}
		// �p���x�𐳋K�����Ċi�[���܂�
		for (int i = 0; i < sizeof(m_angular_velocity) / sizeof(m_angular_velocity[0]); i++) {
			m_angular_velocity[i] = accelerate(m_rawdata.angular[i]);
		}
//TRACE("%f %f %f\n", m_accelerate[0], m_accelerate[1], m_accelerate[2]);
		if (m_logger.handler != NULL) {
			// LOG �������Ăяo���܂�
			if (!m_logger.handler(m_logger.parameter)) {
				m_logger.handler = NULL;
			}
		}
		break;
	case 0x88:	// �v���J�n�ʒm
//dump(&m_rxbuffer[2], 2);
		break;
	// ���X�|���X
	case 0x8f:	// �R�}���h���X�|���X
		if (m_rxbuffer[2] != 0) {
			m_bError = true;	// �R�}���h�G���[
		}
		break;
	case 0x93:	// �v����������
//TRACE("%d\n", m_rxbuffer[2]); dumpdate(&m_rxbuffer[3]); dumpdate(&m_rxbuffer[9]);
		break;
/*
	// �C�x���g
	case 0x80:	// �����x�p���x�v���f�[�^�ʒm
	case 0x81:	// �n���C�v���f�[�^�ʒm
	case 0x82:	// �C���v���f�[�^�ʒm
	case 0x83:	// �o�b�e���d���f�[�^�ʒm
	case 0x84:	// �O���g���[�q�f�[�^�ʒm
	case 0x85:	// �O���g���[�q�G�b�W���o�ʒm
	case 0x86:	// �O���g��I2C��M�f�[�^�ʒm
	case 0x87:	// �v���G���[�ʒm
	case 0x88:	// �v���J�n�ʒm
	case 0x89:	// �v���I���ʒm
	// ���X�|���X
	case 0x8f:	// �R�}���h���X�|���X
	case 0x90:	// �@����擾����
	case 0x92:	// �����擾����
	case 0x93:	// �v����������
	case 0x97:	// ����/�p���x�v���ݒ艞��
	case 0x99:	// �n���C�v���ݒ艞��
	case 0x9b:	// �C���v���ݒ艞��
	case 0x9d:	// �o�b�e���d���v���ݒ艞��
	case 0x9f:	// �O���g���[�q�v��&�G�b�W�f�[�^�o�͐ݒ艞��
	case 0xa1:	// �O���g��I2C�ʐM�ݒ艞��
	case 0xa3:	// �����x�Z���T�v�������W�ݒ艞��
	case 0xa6:	// �p���x�Z���T�v�������W�ݒ艞��
	case 0xaa:	// �O���g��I2C�ʐM�f�o�C�X�ݒ艞��
	case 0xab:	// �O���g��I2C�ʐM�e�X�g����
	case 0xad:	// �I�v�V�����{�^�����샂�[�h�ݒ艞��
	case 0xaf:	// �v���L�^�㏑���ݒ艞��
	case 0xb1:	// �O���g���[�q�ݒ艞��
	case 0xb3:	// �u�U�[���ʐݒ艞��
	case 0xb6:	// �v���f�[�^�L�^�G���g����������
	case 0xb7:	// �v���f�[�^�L�^�G���g������
	case 0xb8:	// �v���f�[�^�L�^�G���g���ڍ׉���
	case 0xb9:	// �v���f�[�^�L�^�������ǂݏo����������
	case 0xba:	// �v���f�[�^�L�^�������c�e�ʉ���
	case 0xbb:	// �o�b�e����ԉ���
	case 0xbc:	// �����ԉ���
	case 0xbd:	// �����x�Z���T�I�t�Z�b�g�l����
	case 0xbe:	// �p���x�Z���T�I�t�Z�b�g�l����
	case 0xd1:	// �I�[�g�p���[�I�t���Ԑݒ�擾����
	case 0xd3:	// �I�t���C���v��Bluetooth �ڑ���t�ݒ�擾����
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