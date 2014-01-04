/*
 * sercfg.c - the serial-port specific parts of the PuTTY
 * configuration box. Centralised as cross-platform code because
 * more than one platform will want to use it, but not part of the
 * main configuration. The expectation is that each platform's
 * local config function will call out to ser_setup_config_box() if
 * it needs to set up the standard serial stuff. (Of course, it can
 * then apply local tweaks after ser_setup_config_box() returns, if
 * it needs to.)
 */

#include <assert.h>
#include <stdlib.h>

#include "putty.h"
#include "dialog.h"
#include "storage.h"

void ser_com_list_handler(union control *ctrl, void *dlg, void *data, int event);

static void serial_parity_handler(union control *ctrl, void *dlg,
				  void *data, int event)
{
    static const struct {
	const char *name;
	int val;
    } parities[] = {
	{"None", SER_PAR_NONE},
	{"Odd", SER_PAR_ODD},
	{"Even", SER_PAR_EVEN},
	{"Mark", SER_PAR_MARK},
	{"Space", SER_PAR_SPACE},
    };
    int mask = ctrl->listbox.context.i;
    int i, j;
    Conf *conf = (Conf *)data;

    if (event == EVENT_REFRESH) {
	/* Fetching this once at the start of the function ensures we
	 * remember what the right value is supposed to be when
	 * operations below cause reentrant calls to this function. */
	int oldparity = conf_get_int(conf, CONF_serparity);

	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < lenof(parities); i++)  {
	    if (mask & (1 << i))
		dlg_listbox_addwithid(ctrl, dlg, parities[i].name,
				      parities[i].val);
	}
	for (i = j = 0; i < lenof(parities); i++) {
	    if (mask & (1 << i)) {
		if (oldparity == parities[i].val) {
		    dlg_listbox_select(ctrl, dlg, j);
		    break;
		}
		j++;
	    }
	}
	if (i == lenof(parities)) {    /* an unsupported setting was chosen */
	    dlg_listbox_select(ctrl, dlg, 0);
	    oldparity = SER_PAR_NONE;
	}
	dlg_update_done(ctrl, dlg);
	conf_set_int(conf, CONF_serparity, oldparity);    /* restore */
    } else if (event == EVENT_SELCHANGE) {
	int i = dlg_listbox_index(ctrl, dlg);
	if (i < 0)
	    i = SER_PAR_NONE;
	else
	    i = dlg_listbox_getid(ctrl, dlg, i);
	conf_set_int(conf, CONF_serparity, i);
    }
}

static void serial_flow_handler(union control *ctrl, void *dlg,
				void *data, int event)
{
    static const struct {
	const char *name;
	int val;
    } flows[] = {
	{"None", SER_FLOW_NONE},
	{"XON/XOFF", SER_FLOW_XONXOFF},
	{"RTS/CTS", SER_FLOW_RTSCTS},
	{"DSR/DTR", SER_FLOW_DSRDTR},
    };
    int mask = ctrl->listbox.context.i;
    int i, j;
    Conf *conf = (Conf *)data;

    if (event == EVENT_REFRESH) {
	/* Fetching this once at the start of the function ensures we
	 * remember what the right value is supposed to be when
	 * operations below cause reentrant calls to this function. */
	int oldflow = conf_get_int(conf, CONF_serflow);

	dlg_update_start(ctrl, dlg);
	dlg_listbox_clear(ctrl, dlg);
	for (i = 0; i < lenof(flows); i++)  {
	    if (mask & (1 << i))
		dlg_listbox_addwithid(ctrl, dlg, flows[i].name, flows[i].val);
	}
	for (i = j = 0; i < lenof(flows); i++) {
	    if (mask & (1 << i)) {
		if (oldflow == flows[i].val) {
		    dlg_listbox_select(ctrl, dlg, j);
		    break;
		}
		j++;
	    }
	}
	if (i == lenof(flows)) {       /* an unsupported setting was chosen */
	    dlg_listbox_select(ctrl, dlg, 0);
	    oldflow = SER_FLOW_NONE;
	}
	dlg_update_done(ctrl, dlg);
	conf_set_int(conf, CONF_serflow, oldflow);/* restore */
    } else if (event == EVENT_SELCHANGE) {
	int i = dlg_listbox_index(ctrl, dlg);
	if (i < 0)
	    i = SER_FLOW_NONE;
	else
	    i = dlg_listbox_getid(ctrl, dlg, i);
	conf_set_int(conf, CONF_serflow, i);
    }
}

void ser_setup_config_box(struct controlbox *b, int midsession,
			  int parity_mask, int flow_mask)
{
    struct controlset *s;
    union control *c;

    if (!midsession) {
	int i;
	extern void config_protocolbuttons_handler(union control *, void *,
						   void *, int);

	/*
	 * Add the serial back end to the protocols list at the
	 * top of the config box.
	 */
	s = ctrl_getset(b, "Session", "hostport",
			"Specify the destination you want to connect to");

/*
        for (i = 0; i < s->ncontrols; i++) {
            c = s->ctrls[i];
	    if (c->generic.type == CTRL_RADIO &&
		c->generic.handler == config_protocolbuttons_handler) {
		c->radio.nbuttons++;
		c->radio.ncolumns++;
		c->radio.buttons =
		    sresize(c->radio.buttons, c->radio.nbuttons, char *);
		c->radio.buttons[c->radio.nbuttons-1] =
		    dupstr("Serial");
		c->radio.buttondata =
		    sresize(c->radio.buttondata, c->radio.nbuttons, intorptr);
		c->radio.buttondata[c->radio.nbuttons-1] = I(PROT_SERIAL);
		if (c->radio.shortcuts) {
		    c->radio.shortcuts =
			sresize(c->radio.shortcuts, c->radio.nbuttons, char);
		    c->radio.shortcuts[c->radio.nbuttons-1] = 'r';
		}
	    }
	}
*/
    }

    /*
     * Entirely new Connection/Serial panel for serial port
     * configuration.
     */
    ctrl_settitle(b, "Connection/Serial",
		  "Options controlling local serial lines");

    if (!midsession) {
	/*
	 * We don't permit switching to a different serial port in
	 * midflight, although we do allow all other
	 * reconfiguration.
	 */
	s = ctrl_getset(b, "Connection/Serial", "serline",
			"Select a serial line");
    
    ctrl_combobox(s, "Serial line to connect to", 'l', 40,
             HELPCTX(serial_line),
             ser_com_list_handler, P(NULL), P(NULL));
    }

    s = ctrl_getset(b, "Connection/Serial", "sercfg", "Configure the serial line");
    ctrl_editbox(s, "Speed (baud)", 's', 40,
		 HELPCTX(serial_speed),
		 conf_editbox_handler, I(CONF_serspeed), I(-1));
    ctrl_editbox(s, "Data bits", 'b', 40,
		 HELPCTX(serial_databits),
		 conf_editbox_handler, I(CONF_serdatabits), I(-1));
    /*
     * Stop bits come in units of one half.
     */
    ctrl_editbox(s, "Stop bits", 't', 40,
		 HELPCTX(serial_stopbits),
		 conf_editbox_handler, I(CONF_serstopbits), I(-2));
    ctrl_droplist(s, "Parity", 'p', 40,
		  HELPCTX(serial_parity),
		  serial_parity_handler, I(parity_mask));
    ctrl_droplist(s, "Flow control", 'f', 40,
		  HELPCTX(serial_flow),
		  serial_flow_handler, I(flow_mask));
}

#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4201)
#include <SetupAPI.h>
#include <devguid.h>
#pragma warning(pop)
#include <stdio.h>

#define MAX_COM 16
struct COM_LIST{
	int count;
	int com_id[MAX_COM];
	char com_name[MAX_COM][256];
	//int com[MAX_COM];
};
static struct COM_LIST com_list;

int get_comm_list()
{
	int it;

	unsigned int com_index=0;
	
	int now_sel=-1;//2013-03-04:表示更新前被选中的设备现已不存在,否则表示ComboBox索引
	int new_sel=0;
	
	//SetupApi
	HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA spdata={0};

	//遍历串口设备,通过SetupApi
	com_list.count = 0;
	hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS,0,0,DIGCF_PRESENT);
	if(hDevInfo == INVALID_HANDLE_VALUE){
		com_list.count = 0;
		return 0;
	}
	spdata.cbSize = sizeof(spdata);
	for(it=0; SetupDiEnumDeviceInfo(hDevInfo,it,&spdata); it++){
		if(SetupDiGetDeviceRegistryProperty(hDevInfo,&spdata,SPDRP_FRIENDLYNAME,
			NULL,(PBYTE)&com_list.com_name[com_index][0],sizeof(com_list.com_name[0]),NULL))
		{
			char* pch = NULL;
			int tmp_id = 0;
			//2013-03-09修复未检测并口的错误
			pch = strstr(&com_list.com_name[com_index][0],"LPT");
			if(pch) continue;//并口
			pch = strstr(&com_list.com_name[com_index][0],"COM");
			__try{
				tmp_id = atoi(pch+3);
				*(pch-1) = 0;
			}
			__except(EXCEPTION_EXECUTE_HANDLER){
			}
			com_list.com_id[com_index] = tmp_id;
			com_index++;
		}
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
	com_list.count = com_index;

	return com_list.count;
}

void ser_com_list_handler(union control *ctrl, void *dlg, void *data, int event)
{
    Conf *conf = (Conf *)data;
    if (event == EVENT_REFRESH) {
    	int i;
    	char *comm;
        char comm_name[256];

    	dlg_update_start(ctrl, dlg);
    	/*
    	 * Some backends may wish to disable the drop-down list on
    	 * this edit box. Be prepared for this.
    	 */
    	if (ctrl->editbox.has_list) {
    	    dlg_listbox_clear(ctrl, dlg);
            get_comm_list();
    	    for (i = 0; i < com_list.count; i++){
                sprintf(comm_name, "COM%d(%s)", com_list.com_id[i], com_list.com_name[i]);
        		dlg_listbox_add(ctrl, dlg, comm_name);
            }
        }
    	comm = conf_get_str(conf, CONF_serline);
    	if (!comm)
    	    comm = "COM1";
    	dlg_editbox_set(ctrl, dlg, comm);
    	dlg_update_done(ctrl, dlg);
    } else if (event == EVENT_VALCHANGE) {
    	char *s = dlg_editbox_get(ctrl, dlg);
	    conf_set_str(conf, CONF_serline, s);
    	sfree(s);
    }
}


