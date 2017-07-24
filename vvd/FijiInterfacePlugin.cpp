#include "stdwx.h"
#include "FijiInterfacePlugin.h"
#include "FijiInterfacePluginWindow.h"
#include "VRenderFrame.h"
#include "compatibility.h"

IMPLEMENT_DYNAMIC_CLASS(SampleGuiPlugin1, wxObject)


FijiServer::~FijiServer(void)
{
	if (m_connection)
	{
		m_connection->Disconnect();
		delete m_connection;
	}
}

wxConnectionBase* FijiServer::OnAcceptConnection(const wxString &topic)
{
	if (topic == _("FijiVVDPlugin"))
	{
		m_connection = new FijiServerConnection;
		m_connection->SetFrame(m_vframe);
		notifyAll(FI_CONNECT);
		return m_connection;
	}
	else
		return NULL;
}

void FijiServerConnection::SetTimeout(long seconds)
{
	if (m_sock)
		m_sock->SetTimeout(seconds);
}

long FijiServerConnection::GetTimeout()
{
	if (m_sock)
		return m_sock->GetTimeout();
	else
		return -1L;
}

bool FijiServerConnection::OnAdvise(const wxString& topic, const wxString& item, char *data,
				                int size, wxIPCFormat format)
{
	wxMessageBox(topic, data);

	return true;
}

bool FijiServerConnection::OnStartAdvise(const wxString& topic, const wxString& item)
{
	wxMessageBox(wxString::Format("OnStartAdvise(\"%s\",\"%s\")", topic.c_str(), item.c_str()));
	return true;
}

bool FijiServerConnection::OnStopAdvise(const wxString& topic, const wxString& item)
{
	wxMessageBox(wxString::Format("OnStopAdvise(\"%s\",\"%s\")", topic.c_str(), item.c_str()));
	return true;
}

bool FijiServerConnection::OnPoke(const wxString& topic, const wxString& item, const void *data, size_t size, wxIPCFormat format)
{
	char ret[1] = {10};
	switch (format)
	{
	case wxIPC_TEXT:
		//wxMessageBox(wxString::Format("OnPoke(\"%s\",\"%s\",\"%s\")", topic.c_str(), item.c_str(), (const char*)data));
		if (item == "version")
		{
			notifyAll(FI_VERSION_CHECK, data, size);
			int newbuffersize = 300*1024*1024;
			m_sock->SetOption(SOL_SOCKET, SO_RCVBUF, &newbuffersize, sizeof(newbuffersize));
			m_sock->SetOption(SOL_SOCKET, SO_SNDBUF, &newbuffersize, sizeof(newbuffersize));
			m_sock->SetFlags(wxSOCKET_BLOCK|wxSOCKET_WAITALL);
			//m_sock->Write(ret, 1);
		}
		if (item == "confirm")
		{
			Poke(item, data, size, wxIPC_TEXT);
			notifyAll(FI_CONFIRM, data, size);
		}
		if (item == "pid")
		{
			notifyAll(FI_PID, data, size);
		}
		if (item == "com_finish")
			notifyAll(FI_COMMAND_FINISHED, data, size);
		break;
	case wxIPC_PRIVATE:
		if (item == "settimeout" && size == 4)
		{
			int sec = *((const int32_t *)data);
			if (sec > 0)
				SetTimeout(sec);
		}
		if (item == "volume")
		{
			notifyAll(FI_VOLUMEDATA, data, size);
//			Poke("volume_received", "recv", 4, wxIPC_TEXT);
		}
	}
	return true;
}

bool FijiServerConnection::OnExec(const wxString &topic, const wxString &data)
{
	return true;
}




SampleGuiPlugin1::SampleGuiPlugin1()
	: wxGuiPluginBase(NULL, NULL), m_server(NULL), m_initialized(false), m_booting(false)
{

}

SampleGuiPlugin1::SampleGuiPlugin1(wxEvtHandler * handler, wxWindow * vvd)
	: wxGuiPluginBase(handler, vvd), m_server(NULL), m_initialized(false), m_booting(false)
{
}

SampleGuiPlugin1::~SampleGuiPlugin1()
{
	wxDELETE(m_server);
}

void SampleGuiPlugin1::doAction(ActionInfo *info)
{
	if (!info)
		return;
	int evid = info->id;

	switch (evid)
	{
	case FI_CONNECT:
		if (m_server && m_server->GetConnection())
			m_server->GetConnection()->addObserver(this);
		m_booting = false;
		break;
	case FI_VERSION_CHECK:
		//wxMessageBox(wxString::Format("VVD plugin ver. %s     Fiji plugin ver. %s", FI_PLUGIN_VERSION, wxString((char *)info->data)));
		m_fiji_plugin_ver = wxString((char *)info->data);
		if (wxString((char *)info->data) != FI_PLUGIN_VERSION)
		{
			//do something
		}
		notifyAll(FI_VERSION_CHECK, info->data, info->size);
		break;
	case FI_PID:
		m_pid = wxString((char *)info->data);
		notifyAll(FI_PID, info->data, info->size);
		break;
	case FI_CONFIRM:
		m_initialized = true;
		break;
	case FI_VOLUMEDATA:
		if (m_vvd)
		{
			int idx = 0;
			const char *ptr = (const char *)info->data;

			VRenderFrame *vframe = (VRenderFrame *)m_vvd;
						
			int name_len = *((const int32_t *)ptr);
			ptr += 4;

			char *name = new char[name_len];
			memcpy(name, ptr, name_len);
			ptr += name_len;

			int nx = *((const int32_t *)ptr);
			ptr += 4;

			int ny = *((const int32_t *)ptr);
			ptr += 4;

			int nz = *((const int32_t *)ptr);
			ptr += 4;

			int bd = *((const int32_t *)ptr);
			ptr += 4;

			int r = *((const int32_t *)ptr);
			ptr += 4;

			int g = *((const int32_t *)ptr);
			ptr += 4;

			int b = *((const int32_t *)ptr);
			ptr += 4;

			double spcx = *((const double *)ptr);
			ptr += 8;

			double spcy = *((const double *)ptr);
			ptr += 8;

			double spcz = *((const double *)ptr);
			ptr += 4;

			VolumeData *vd = new VolumeData();
			vd->AddEmptyData(bd, nx, ny, nz, spcx, spcy, spcz);
			FLIVR::Color col((double)r/255.0, (double)g/255.0, (double)b/255.0);
			vd->SetName(wxString(name));
			vd->SetColor(col);
			vd->SetBaseSpacings(spcx, spcy, spcz);
			vd->SetSpcFromFile(true);

			DataManager *dm = vframe->GetDataManager();
			if (dm) dm->SetVolumeDefault(vd);

			Nrrd *nrrd = vd->GetVolume(false);
			memcpy(nrrd->data, ptr, nx*ny*nz*(bd/8));
			//notifyAll(FI_VOLUMEDATA, name, name_len);

			vframe->AddVolume(vd, NULL);

			delete [] name;
		}
		break;
	case FI_COMMAND_FINISHED:
		notifyAll(FI_COMMAND_FINISHED);
		break;
	}
}

bool SampleGuiPlugin1::SendCommand(wxString command)
{
	if (!m_server || !m_server->GetConnection())
		return false;
	return m_server->GetConnection()->Poke(_("com"), command);
}

bool SampleGuiPlugin1::SendCommand(wxString command, const void * data)
{
	return true;
}

wxString SampleGuiPlugin1::GetName() const
{
	return _("Fiji Interface");
}

wxString SampleGuiPlugin1::GetId() const
{
	return wxT("{4E97DF66-5FBB-4719-AF17-76C1C82D3FE1}");
}

wxWindow * SampleGuiPlugin1::CreatePanel(wxWindow * parent)
{
	return new SampleGuiPluginWindow1(this, parent);
}

void SampleGuiPlugin1::StartFiji()
{
	if (m_fiji_path.IsEmpty() || m_initialized || m_booting)
		return;

	m_initialized = false;
	wxString command = m_fiji_path + " -port774 -macro vvd_listener.ijm";
	m_booting = true;
	wxExecute(command);
}

void SampleGuiPlugin1::CloseFiji()
{
	if (m_fiji_path.IsEmpty())
		return;

	wxString command = m_fiji_path + " -port774 -macro vvd_quit.ijm";
	wxExecute(command);
}

bool SampleGuiPlugin1::isReady()
{
	if (!m_server || !m_server->GetConnection() || !m_server->GetConnection()->GetConnected())
		return false;

	return m_initialized;
}

void SampleGuiPlugin1::OnInit()
{
	LoadConfigFile();
	m_server = new FijiServer;
	m_server->Create("8002");
	m_server->addObserver(this);

	StartFiji();
}

void SampleGuiPlugin1::OnDestroy()
{
	CloseFiji();
}

void SampleGuiPlugin1::LoadConfigFile()
{
	wxString expath = wxStandardPaths::Get().GetExecutablePath();
	expath = expath.BeforeLast(GETSLASH(), NULL);
#ifdef _WIN32
	wxString dft = expath + "\\fiji_interface_settings.dft";
	if (!wxFileExists(dft))
		dft = wxStandardPaths::Get().GetUserConfigDir() + "\\fiji_interface_settings.dft";
#else
	wxString dft = expath + "/../Resources/fiji_interface_settings.dft";
#endif
	if (wxFileExists(dft))
	{
		wxFileInputStream is(dft);
		if (is.IsOk())
		{
			wxFileConfig fconfig(is);
			wxString str;
			if (fconfig.Read("fiji_path", &str))
				m_fiji_path = str;
		}
	}
}

void SampleGuiPlugin1::SaveConfigFile()
{
	wxFileConfig fconfig("Fiji interface default settings");

	fconfig.Write("fiji_path", m_fiji_path);

	wxString expath = wxStandardPaths::Get().GetExecutablePath();
	expath = expath.BeforeLast(GETSLASH(),NULL);
#ifdef _WIN32
	wxString dft = expath + "\\fiji_interface_settings.dft";
	wxString dft2 = wxStandardPaths::Get().GetUserConfigDir() + "\\fiji_interface_settings.dft";
	if (!wxFileExists(dft) && wxFileExists(dft2))
		dft = dft2;
#else
	wxString dft = expath + "/../Resources/fiji_interface_settings.dft";
#endif
	wxFileOutputStream os(dft);
	fconfig.Save(os);
}
