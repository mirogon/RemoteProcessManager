#include "remote_process_manager.h"

namespace m1
{
	const int PROCESS_PACKETS_IN_PACKET = 1400 / sizeof(m1::network_packet_process);

	remote_process_manager::remote_process_manager():
		logger{"debug_log.txt", false}
	{

		logger.log("Start of program\n");

		m_traymenu = new QMenu();

		QIcon icon("data\\img\\logo.png");
		this->setIcon(icon);
		this->setToolTip("Remote Process Manager");
		this->setVisible(true);

		//Add information about the current server address to the tray menu
		QString local_ip;
		QList<QHostAddress> local_ips = QNetworkInterface::allAddresses();

		for (int i = 0; i < local_ips.size(); ++i)
		{
			if (local_ips.at(i) != QHostAddress::LocalHost && local_ips.at(i).toIPv4Address())
			{
				local_ip = local_ips.at(i).toString();
			}
		}

		if (local_ip.isEmpty())
		{
			local_ip = (QHostAddress(QHostAddress::LocalHost)).toString();
		}

		QString s = "Server address: " + local_ip;

		m_trayaction_connect = m_traymenu->addAction(s);

		//Add exit button
		m_trayaction_exit = m_traymenu->addAction("Exit", QApplication::instance(), QApplication::exit);
		this->setContextMenu(m_traymenu);
		
		m_socket_find = new QUdpSocket();
		m_socket_find->bind(55557);

		m_server = new QTcpServer(this);
		
		connect( m_server, &QTcpServer::newConnection, this,  &remote_process_manager::client_connected  );

		if (m_server->listen(QHostAddress::Any, 55556) == false)
		{
			logger.log("Server failed to listen to port 55556");
		}

		receive_frequency = 1000;
		send_frequency = 2000;

		m_timer_find = new QTimer();
		connect(m_timer_find, &QTimer::timeout, this, &remote_process_manager::find_client);

		m_timer_find->start(500);

		m_timer_receive = new QTimer();
		connect(m_timer_receive, &QTimer::timeout, this, &remote_process_manager::receive_packets);

		m_timer_send = new QTimer();
		connect(m_timer_send, &QTimer::timeout, this, &remote_process_manager::send_packets);

		logger.log("End of constructor\n");

	}

	remote_process_manager::~remote_process_manager()
	{
		m_trayaction_connect->deleteLater();
		m_trayaction_exit->deleteLater();
		m_traymenu->deleteLater();
		m_timer_find->deleteLater();
		m_timer_receive->deleteLater();
		m_timer_send->deleteLater();
		m_socket_find->deleteLater();
		m_socket->deleteLater();
		m_server->deleteLater();
	}

	void remote_process_manager::client_connected()
	{
		logger.log("New client connection!");

		m_socket = m_server->nextPendingConnection();
		connect(m_socket, &QTcpSocket::disconnected, this, &remote_process_manager::client_disconnected);

		m_timer_find->stop();
		m_timer_receive->start(receive_frequency);
		m_timer_send->start(send_frequency);
	}

	void remote_process_manager::client_disconnected()
	{
		m_socket->abort();
		m_timer_find->start();
	}
	
	void remote_process_manager::receive_packets()
	{
		logger.log("trying to receive packets");
		if (m_socket != nullptr)
		{
			if (m_socket->state() == QTcpSocket::ConnectedState)
			{
				while (m_socket->bytesAvailable() >= sizeof(network_packet_kill))
				{
					static network_packet_kill received_packet;
					static char buffer[sizeof(network_packet_kill)];

					m_socket->read(&buffer[0], sizeof(network_packet_kill));

					memcpy(&received_packet, buffer, sizeof(network_packet_kill));
					logger.log("received kill package");
					m_process_manager.kill_process(received_packet.process_id);
				}
			}
		}
		//timer_receive->start(receive_frequency);
	}
	
	void remote_process_manager::find_client()
	{
		if (m_socket_find)
		{
			if (m_socket_find->hasPendingDatagrams())
			{
				//Reply with packet_id 9 and computername SIZE OF PACKET = sizeof(packet_identifier) + MAX_COMPUTERNAME_LENGTH + 1 = 20
				QNetworkDatagram dg = m_socket_find->receiveDatagram();
				QByteArray reply;

				packet_identifier id = NETWORK_PACKET_FIND;

				char cache[MAX_COMPUTERNAME_LENGTH + 1];
				memcpy(cache, &id, sizeof(packet_identifier));

				reply.append(cache, sizeof(packet_identifier));

				static unsigned long l = MAX_COMPUTERNAME_LENGTH + 1;
				GetComputerNameA(cache, &l);

				reply.append(cache, MAX_COMPUTERNAME_LENGTH + 1);

				m_socket_find->writeDatagram(reply, dg.senderAddress(), dg.senderPort());
			}
		}
	}

	void remote_process_manager::send_packets()
	{

		logger.log("Trying to send packets!\n");

		m_process_manager.init_processes_THL();

		static std::vector<m1::process> processes;
		processes = m_process_manager.get_processes();

		static unsigned int num_processes;
		num_processes = processes.size();

		static int loop;
		loop = 0;

		while (num_processes >= PROCESS_PACKETS_IN_PACKET)
		{
			num_processes -= PROCESS_PACKETS_IN_PACKET;

			static network_packet_process packet[PROCESS_PACKETS_IN_PACKET];
			static char buffer[sizeof(packet)];

			//Initialize network packet from m1::packet's
			for (int i = 0; i < PROCESS_PACKETS_IN_PACKET; ++i)
			{
				if (i == 0)
				{
					packet[i].packet_id = NETWORK_PACKET_PROCESS_FIRST;
				}
				else if (i == 19 && num_processes == 0)
				{
					packet[i].packet_id = NETWORK_PACKET_PROCESS_LAST;
				}
				else
				{
					packet[i].packet_id = NETWORK_PACKET_PROCESS;
				}
				packet[i].process_id = processes.at(i + loop * PROCESS_PACKETS_IN_PACKET).process_id;
				strcpy(packet[i].process_name, processes.at(i + loop * PROCESS_PACKETS_IN_PACKET).name.c_str());
			}

			//Send packet
			memcpy(buffer, packet, sizeof(packet));
			if (m_socket != nullptr && m_socket->state() == QTcpSocket::ConnectedState)
			{
				logger.log("sending 20 processes package!\n");
				m_socket->write(buffer, sizeof(packet));
			}
			++loop;
		}
		
		if (num_processes > 0)
		{
			std::unique_ptr<network_packet_process[]> packet{ new network_packet_process[num_processes] };
			std::unique_ptr<char[]> buffer{ new char[sizeof(network_packet_process) * num_processes] };

			//Initialize network packet from m1::packet's
			for (int i = 0; i < num_processes; ++i)
			{
				if (i == num_processes - 1)
				{
					(packet.get())[i].packet_id = NETWORK_PACKET_PROCESS_LAST;
				}
				else
				{
					(packet.get())[i].packet_id = NETWORK_PACKET_PROCESS;
				}
				(packet.get())[i].process_id = processes.at(i + loop * PROCESS_PACKETS_IN_PACKET).process_id;
				strcpy((packet.get())[i].process_name, processes.at(i + loop * PROCESS_PACKETS_IN_PACKET).name.c_str());
			}

			//Send packet
			memcpy(buffer.get(), packet.get(), sizeof(network_packet_process) * num_processes);
			if (m_socket != nullptr && m_socket->state() == QTcpSocket::ConnectedState)
			{
				logger.log("sending package!");
				m_socket->write(buffer.get(), sizeof(network_packet_process) * num_processes);
			}
		}
	}
	
};