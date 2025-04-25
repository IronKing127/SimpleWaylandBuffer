#include <iostream>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include "xdg-shell.h"

wl_compositor* compositor;

xdg_wm_base* xdgBase;

wl_shm* wlshm;

void globalRegistryHandler(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    std::string s_interface = std::string(interface);
	std::cout << s_interface << std::endl;
    if(s_interface == "wl_compositor")
        compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, version));

    else if(s_interface == "xdg_wm_base")
        xdgBase = static_cast<xdg_wm_base*>(wl_registry_bind(registry, id, &xdg_wm_base_interface, version));

    else if(s_interface == "wl_shm")
        wlshm = static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, version));
}

wl_registry_listener registryListener = {globalRegistryHandler};

int main()
{
	wl_display* display = wl_display_connect(NULL);

	wl_registry* m_registry = wl_display_get_registry(display);
	wl_registry_add_listener(m_registry, &registryListener, NULL);

	wl_display_roundtrip(display);
	wl_display_dispatch(display);


}
