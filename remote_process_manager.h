#pragma once
//Core
#include <memory>
//Windows

//M1
#include <M1Log.h>

//Qt
#include <qsystemtrayicon.h>
#include <qtcpserver.h>
#include <qudpsocket.h>
#include <qbytearray.h>
#include <qtcpsocket.h>
#include <qnetworkdatagram>
#include <QAction>
#include <QMenu>
#include <qapplication.h>
#include <qnetworkinterface.h>
#include <qtimer>

#include "process_manager.h"


namespace m1
{
	class remote_process_manager : public QSystemTrayIcon
	{
	public:

		remote_process_manager();
		~remote_process_manager();

	public slots:

		void client_connected();
		void client_disconnected();

		void receive_packets();
	
		void find_client();

	private /*methods*/:
		void send_packets();
	
	private:

		m1::process_manager m_process_manager;

		QMenu* m_traymenu;

		QAction* m_trayaction_exit;
		QAction* m_trayaction_connect;

		QUdpSocket* m_socket_find;
		QTimer* m_timer_find;

		QTcpServer* m_server;
		QTcpSocket* m_socket;

		QTimer* m_timer_receive;
		unsigned int receive_frequency;

		QTimer* m_timer_send;
		unsigned int send_frequency;

		m1::M1Log logger;

	};

	enum e_packet_identifier{NETWORK_PACKET_PROCESS_FIRST = 5, NETWORK_PACKET_PROCESS = 6, NETWORK_PACKET_PROCESS_LAST = 7, NETWORK_PACKET_KILL = 8, NETWORK_PACKET_FIND = 9};
	typedef uint32_t packet_identifier; 

	struct network_packet_process
	{
		packet_identifier packet_id;
		uint32_t process_id;
		char process_name[64];
	};

	struct network_packet_kill
	{
		packet_identifier packet_id;
		uint32_t process_id;
	};

};
