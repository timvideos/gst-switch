dbus-send \
 	--address="tcp:host=127.0.0.1,port=5000" \
 	--print-reply=literal \
 	--dest="info.duzy.gst_switch.SwitchClientInterface" \
 	/info/duzy/gst_switch/SwitchController \
 	info.duzy.gst_switch.SwitchControllerInterface.get_compose_port

gdbus introspect --address tcp:host=127.0.0.1,port=5000 \
				 --dest info.duzy.gst.switch.SwitchUIInterface \
				 --object-path /info/duzy/gst/switch/SwitchUI
