#include "HyprCtl.hpp"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <json/json.h>

std::string monitorsRequest(bool json) {
    if (json) {
        Json::Value result = Json::Value(Json::arrayValue);
        for (auto& m : g_pCompositor->m_lMonitors) {
            Json::Value data = Json::Value(Json::objectValue);

            data["name"] = m.szName;
            data["id"] = m.ID;
            data["size_x"] = (int)m.vecSize.x;
            data["size_y"] = (int)m.vecSize.y;
            data["refresh_rate"] = m.refreshRate;
            data["pos_x"] = (int)m.vecPosition.x;
            data["pos_y"] = (int)m.vecPosition.y;
            data["active_workspace_id"] = m.activeWorkspace;
            data["active_workspace_name"] = g_pCompositor->getWorkspaceByID(m.activeWorkspace)->m_szName;
            data["reserved_top_left_x"] = (int)m.vecReservedTopLeft.x;
            data["reserved_top_left_y"] = (int)m.vecReservedTopLeft.y;
            data["reserved_bottom_right_x"] = (int)m.vecReservedBottomRight.x;
            data["reserved_bottom_right_y"] = (int)m.vecReservedBottomRight.y;
            
            result.append(data);
        }

        Json::FastWriter writer;
        return writer.write(result);
    }
    else {
        std::string result = "";
        for (auto& m : g_pCompositor->m_lMonitors) {
            result += getFormat("Monitor %s (ID %i):\n\t%ix%i@%f at %ix%i\n\tactive workspace: %i (%s)\n\treserved: %i %i %i %i\n\n",
                                m.szName.c_str(), m.ID, (int)m.vecSize.x, (int)m.vecSize.y, m.refreshRate, (int)m.vecPosition.x, (int)m.vecPosition.y, m.activeWorkspace, g_pCompositor->getWorkspaceByID(m.activeWorkspace)->m_szName.c_str(), (int)m.vecReservedTopLeft.x, (int)m.vecReservedTopLeft.y, (int)m.vecReservedBottomRight.x, (int)m.vecReservedBottomRight.y);
        }

        return result;
    }
}

std::string clientsRequest(bool json) {
    if (json) {
        Json::Value result = Json::Value(Json::arrayValue);
        for (auto& w : g_pCompositor->m_lWindows) {
            if (w.m_bIsMapped) {
                Json::Value data = Json::Value(Json::objectValue);

                char buf[8] = "";
                snprintf(buf, sizeof buf, "%x", &w);
                data["address"] = std::string(strdup(buf));

                data["title"] = w.m_szTitle;
                data["pos_x"] = (int)w.m_vRealPosition.vec().x;
                data["pos_y"] = (int)w.m_vRealPosition.vec().y;
                data["size_x"] = (int)w.m_vRealSize.vec().x;
                data["size_y"] = (int)w.m_vRealSize.vec().y;
                data["workspace_id"] = w.m_iWorkspaceID;
                data["workspace_name"] = (w.m_iWorkspaceID == -1 ? "" : g_pCompositor->getWorkspaceByID(w.m_iWorkspaceID) ? g_pCompositor->getWorkspaceByID(w.m_iWorkspaceID)->m_szName.c_str() : std::string("Invalid workspace " + std::to_string(w.m_iWorkspaceID)).c_str());
                data["floating"] = w.m_bIsFloating;
                data["monitor"] = w.m_iMonitorID;
                data["class"] = g_pXWaylandManager->getAppIDClass(&w).c_str();
                
                result.append(data);
            }
        }

        Json::FastWriter writer;
        return writer.write(result);
    }
    else {
        std::string result = "";
        for (auto& w : g_pCompositor->m_lWindows) {
            if (w.m_bIsMapped)
                result += getFormat("Window %x -> %s:\n\tat: %i,%i\n\tsize: %i,%i\n\tworkspace: %i (%s)\n\tfloating: %i\n\tmonitor: %i\n\tclass: %s\n\n",
                                &w, w.m_szTitle.c_str(), (int)w.m_vRealPosition.vec().x, (int)w.m_vRealPosition.vec().y, (int)w.m_vRealSize.vec().x, (int)w.m_vRealSize.vec().y, w.m_iWorkspaceID, (w.m_iWorkspaceID == -1 ? "" : g_pCompositor->getWorkspaceByID(w.m_iWorkspaceID) ? g_pCompositor->getWorkspaceByID(w.m_iWorkspaceID)->m_szName.c_str() : std::string("Invalid workspace " + std::to_string(w.m_iWorkspaceID)).c_str()), (int)w.m_bIsFloating, w.m_iMonitorID, g_pXWaylandManager->getAppIDClass(&w).c_str());
        }
        return result;
    }
}

std::string workspacesRequest(bool json) {
    if (json) {
        Json::Value result = Json::Value(Json::arrayValue);
        for (auto& w : g_pCompositor->m_lWorkspaces) {
            Json::Value data = Json::Value(Json::objectValue);

            data["id"] = w.m_iID;
            data["name"] = w.m_szName;
            data["monitor_name"] = g_pCompositor->getMonitorFromID(w.m_iMonitorID)->szName;
            data["window_count"] = g_pCompositor->getWindowsOnWorkspace(w.m_iID);
            data["has_fullscreen"] = (int)w.m_bHasFullscreenWindow;
            
            result.append(data);
        }

        Json::FastWriter writer;
        return writer.write(result);
    }
    else {
        std::string result = "";
        for (auto& w : g_pCompositor->m_lWorkspaces) {
            result += getFormat("workspace ID %i (%s) on monitor %s:\n\twindows: %i\n\thasfullscreen: %i\n\n",
                                w.m_iID, w.m_szName.c_str(), g_pCompositor->getMonitorFromID(w.m_iMonitorID)->szName.c_str(), g_pCompositor->getWindowsOnWorkspace(w.m_iID), (int)w.m_bHasFullscreenWindow);
        }
        return result;
    }
}

std::string activeWindowRequest(bool json) {
    const auto PWINDOW = g_pCompositor->m_pLastWindow;
    if (!g_pCompositor->windowValidMapped(PWINDOW))
        return "Invalid";

    if (json) {
        Json::Value result = Json::Value(Json::objectValue);

        char buf[8] = "";
        snprintf(buf, sizeof buf, "%x", PWINDOW);
        result["address"] = std::string(strdup(buf));

        result["title"] = PWINDOW->m_szTitle;
        result["pos_x"] = (int)PWINDOW->m_vRealPosition.vec().x;
        result["pos_y"] = (int)PWINDOW->m_vRealPosition.vec().y;
        result["size_x"] = (int)PWINDOW->m_vRealSize.vec().x;
        result["size_y"] = (int)PWINDOW->m_vRealSize.vec().y;
        result["workspace_id"] = PWINDOW->m_iWorkspaceID;
        result["workspace_name"] = (PWINDOW->m_iWorkspaceID == -1 ? "" : g_pCompositor->getWorkspaceByID(PWINDOW->m_iWorkspaceID)->m_szName.c_str());
        result["floating"] = PWINDOW->m_bIsFloating;
        result["monitor"] = PWINDOW->m_iMonitorID;
        result["class"] = g_pXWaylandManager->getAppIDClass(PWINDOW).c_str();

        Json::FastWriter writer;
        return writer.write(result);
    }
    else {
        return getFormat("Window %x -> %s:\n\tat: %i,%i\n\tsize: %i,%i\n\tworkspace: %i (%s)\n\tfloating: %i\n\tmonitor: %i\n\tclass: %s\n\n",
                            PWINDOW, PWINDOW->m_szTitle.c_str(), (int)PWINDOW->m_vRealPosition.vec().x, (int)PWINDOW->m_vRealPosition.vec().y, (int)PWINDOW->m_vRealSize.vec().x, (int)PWINDOW->m_vRealSize.vec().y, PWINDOW->m_iWorkspaceID, (PWINDOW->m_iWorkspaceID == -1 ? "" : g_pCompositor->getWorkspaceByID(PWINDOW->m_iWorkspaceID)->m_szName.c_str()), (int)PWINDOW->m_bIsFloating, (int)PWINDOW->m_iMonitorID, g_pXWaylandManager->getAppIDClass(PWINDOW).c_str());
    }
}

std::string layersRequest() {
    std::string result = "";

    for (auto& mon : g_pCompositor->m_lMonitors) {
        result += getFormat("Monitor %s:\n");
        int layerLevel = 0;
        for (auto& level : mon.m_aLayerSurfaceLists) {
            result += getFormat("\tLayer level %i:\n", layerLevel);

            for (auto& layer : level) {
                result += getFormat("\t\tLayer %x: xywh: %i %i %i %i\n", layer, layer->geometry.x, layer->geometry.y, layer->geometry.width, layer->geometry.height);
            }

            layerLevel++;
        }
        result += "\n\n";
    }

    return result;
}

std::string devicesRequest(bool json) {
    if (json) {
        Json::Value result = Json::Value(Json::objectValue);

        char buf[8] = "";

        Json::Value mice = Json::Value(Json::arrayValue);

        for (auto& m : g_pInputManager->m_lMice) {
            Json::Value device = Json::Value(Json::objectValue);

            snprintf(buf, sizeof buf, "%x", &m);
            device["address"] = std::string(strdup(buf));
            device["name"] = m.mouse->name;
            
            mice.append(device);
        }

        Json::Value keyboards = Json::Value(Json::arrayValue);

        for (auto& k : g_pInputManager->m_lKeyboards) {
            Json::Value device = Json::Value(Json::objectValue);

            snprintf(buf, sizeof buf, "%x", &k);
            device["address"] = std::string(strdup(buf));
            device["name"] = k.keyboard->name;
            
            keyboards.append(device);
        }

        Json::Value tablets = Json::Value(Json::arrayValue);

        for (auto& d : g_pInputManager->m_lTabletPads) {
            Json::Value device = Json::Value(Json::objectValue);

            device["type"] = "pad";
            
            snprintf(buf, sizeof buf, "%x", &d);
            device["address"] = std::string(strdup(buf));
            
            snprintf(buf, sizeof buf, "%x", d.pTabletParent);
            device["parent_address"] = std::string(strdup(buf));

            device["parent_name"] = d.pTabletParent ? d.pTabletParent->wlrDevice ? d.pTabletParent->wlrDevice->name : "" : "";
            
            tablets.append(device);
        }

        for (auto& d : g_pInputManager->m_lTablets) {
            Json::Value device = Json::Value(Json::objectValue);

            device["type"] = "tablet";
            
            snprintf(buf, sizeof buf, "%x", &d);
            device["address"] = std::string(strdup(buf));
            device["name"] = d.wlrDevice ? d.wlrDevice->name : "";

            tablets.append(device);
        }

        for (auto& d : g_pInputManager->m_lTabletTools) {
            Json::Value device = Json::Value(Json::objectValue);

            device["type"] = "tool";
            
            snprintf(buf, sizeof buf, "%x", &d);
            device["address"] = std::string(strdup(buf));
            
            snprintf(buf, sizeof buf, "%x", d.wlrTabletTool);
            device["tool_data"] = d.wlrTabletTool ? d.wlrTabletTool->data : 0;

            tablets.append(device);
        }

        result["mice"] = mice;
        result["keyboards"] = keyboards;
        result["tablets"] = tablets;

        Json::FastWriter writer;
        return writer.write(result);
    }
    else {
        std::string result = "";

        result += "mice:\n";

        for (auto& m : g_pInputManager->m_lMice) {
            result += getFormat("\tMouse at %x:\n\t\t%s\n", &m, m.mouse->name);
        }

        result += "\n\nKeyboards:\n";

        for (auto& k : g_pInputManager->m_lKeyboards) {
            result += getFormat("\tKeyboard at %x:\n\t\t%s\n", &k, k.keyboard->name);
        }

        result += "\n\nTablets:\n";

        for (auto& d : g_pInputManager->m_lTabletPads) {
            result += getFormat("\tTablet Pad at %x (belongs to %x -> %s)\n", &d, d.pTabletParent, d.pTabletParent ? d.pTabletParent->wlrDevice ? d.pTabletParent->wlrDevice->name : "" : "");
        }

        for (auto& d : g_pInputManager->m_lTablets) {
            result += getFormat("\tTablet at %x:\n\t\t%s\n", &d, d.wlrDevice ? d.wlrDevice->name : "");
        }

        for (auto& d : g_pInputManager->m_lTabletTools) {
            result += getFormat("\tTablet Tool at %x (belongs to %x)\n", &d, d.wlrTabletTool ? d.wlrTabletTool->data : 0);
        }

        return result;
    }
}

std::string cursorPositionRequest(bool json) {
    const int x = (int)g_pCompositor->m_sWLRCursor->x;
    const int y = (int)g_pCompositor->m_sWLRCursor->y;
    if (json) {
        Json::Value result = Json::Value(Json::objectValue);

        result["x"] = x;
        result["y"] = y;

        Json::FastWriter writer;
        return writer.write(result);
    }
    else {
        return getFormat("%ix%i\n", x, y);
    }
}

std::string versionRequest() {
    std::string result = "Hyprland, built from branch " + std::string(GIT_BRANCH) + " at commit " + GIT_COMMIT_HASH + GIT_DIRTY + " (" + GIT_COMMIT_MESSAGE + ").\nflags: (if any)\n";

#ifdef LEGACY_RENDERER
    result += "legacyrenderer\n";
#endif
#ifndef NDEBUG
    result += "debug\n";
#endif
#ifdef NO_XWAYLAND
    result += "no xwayland\n";
#endif

    return result;
}

std::string dispatchRequest(std::string in) {
    // get rid of the dispatch keyword
    in = in.substr(in.find_first_of(' ') + 1);

    const auto DISPATCHSTR = in.substr(0, in.find_first_of(' '));

    const auto DISPATCHARG = in.substr(in.find_first_of(' ') + 1);

    const auto DISPATCHER = g_pKeybindManager->m_mDispatchers.find(DISPATCHSTR);
    if (DISPATCHER == g_pKeybindManager->m_mDispatchers.end())
        return "Invalid dispatcher";

    DISPATCHER->second(DISPATCHARG);

    Debug::log(LOG, "Hyprctl: dispatcher %s : %s", DISPATCHSTR.c_str(), DISPATCHARG.c_str());

    return "ok";
}

std::string dispatchKeyword(std::string in) {
    // get rid of the keyword keyword
    in = in.substr(in.find_first_of(' ') + 1);

    const auto COMMAND = in.substr(0, in.find_first_of(' '));

    const auto VALUE = in.substr(in.find_first_of(' ') + 1);

    std::string retval = g_pConfigManager->parseKeyword(COMMAND, VALUE, true);

    if (COMMAND == "monitor")
        g_pConfigManager->m_bWantsMonitorReload = true; // for monitor keywords

    if (COMMAND.find("input") != std::string::npos)
        g_pInputManager->setKeyboardLayout(); // update kb layout

    Debug::log(LOG, "Hyprctl: keyword %s : %s", COMMAND.c_str(), VALUE.c_str());

    if (retval == "") 
        return "ok";

    return retval;
}

std::string reloadRequest() {
    g_pConfigManager->m_bForceReload = true;

    return "ok";
}

std::string getReply(std::string);

std::string dispatchBatch(std::string request) {
    // split by ;

    request = request.substr(9);
    std::string curitem = "";
    std::string reply = "";

    auto nextItem = [&]() {
        auto idx = request.find_first_of(';');

        if (idx != std::string::npos) {
            curitem = request.substr(0, idx);
            request = request.substr(idx + 1);
        } else {
            curitem = request;
            request = "";
        }

        curitem = removeBeginEndSpacesTabs(curitem);
    };

    nextItem();

    while (curitem != "") {
        reply += getReply(curitem);

        nextItem();
    }

    return reply;
}

std::string getReply(std::string request) {
    if (request == "monitors")
        return monitorsRequest(false);
    else if (request == "monitorsjson")
        return monitorsRequest(true);
    else if (request == "workspaces")
        return workspacesRequest(false);
    else if (request == "workspacesjson")
        return workspacesRequest(true);
    else if (request == "clients")
        return clientsRequest(false);
    else if (request == "clientsjson")
        return clientsRequest(true);
    else if (request == "activewindow")
        return activeWindowRequest(false);
    else if (request == "activewindowjson")
        return activeWindowRequest(true);
    else if (request == "layers")
        return layersRequest();
    else if (request == "version")
        return versionRequest();
    else if (request == "reload")
        return reloadRequest();
    else if (request == "devices")
        return devicesRequest(false);
    else if (request == "devicesjson")
        return devicesRequest(true);
    else if (request == "cursorposition")
        return cursorPositionRequest(false);
    else if (request == "cursorpositionjson")
        return cursorPositionRequest(true);
    else if (request.find("dispatch") == 0)
        return dispatchRequest(request);
    else if (request.find("keyword") == 0)
        return dispatchKeyword(request);
    else if (request.find("[[BATCH]]") == 0)
        return dispatchBatch(request);

    return "unknown request";
}

void HyprCtl::tickHyprCtl() {
    if (!requestMade)
        return;

    std::string reply = "";

    try {
        reply = getReply(request);
    } catch (std::exception& e) {
        Debug::log(ERR, "Error in request: %s", e.what());
        reply = "Err: " + std::string(e.what());
    }

    request = reply;

    requestMade = false;
    requestReady = true;
}

std::string getRequestFromThread(std::string rq) {
    while (HyprCtl::request != "" || HyprCtl::requestMade || HyprCtl::requestReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    HyprCtl::request = rq;
    HyprCtl::requestMade = true;

    while (!HyprCtl::requestReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    HyprCtl::requestReady = false;
    HyprCtl::requestMade = false;

    std::string toReturn = HyprCtl::request;

    HyprCtl::request = "";

    return toReturn;
}

void HyprCtl::startHyprCtlSocket() {
    std::thread([&]() {
        const auto SOCKET = socket(AF_UNIX, SOCK_STREAM, 0);

        if (SOCKET < 0) {
            Debug::log(ERR, "Couldn't start the Hyprland Socket. (1) IPC will not work.");
            return;
        }

        sockaddr_un SERVERADDRESS = {.sun_family = AF_UNIX};

        std::string socketPath = "/tmp/hypr/" + g_pCompositor->m_szInstanceSignature + "/.socket.sock";

        strcpy(SERVERADDRESS.sun_path, socketPath.c_str());

        bind(SOCKET, (sockaddr*)&SERVERADDRESS, SUN_LEN(&SERVERADDRESS));

        // 10 max queued.
        listen(SOCKET, 10);

        sockaddr_in clientAddress;
        socklen_t clientSize = sizeof(clientAddress);

        char readBuffer[1024] = {0};

        Debug::log(LOG, "Hypr socket started at %s", socketPath.c_str());

        while(1) {
            const auto ACCEPTEDCONNECTION = accept(SOCKET, (sockaddr*)&clientAddress, &clientSize);

            if (ACCEPTEDCONNECTION < 0) {
                Debug::log(ERR, "Couldn't listen on the Hyprland Socket. (3) IPC will not work.");
                break;
            }

            auto messageSize = read(ACCEPTEDCONNECTION, readBuffer, 1024);
            readBuffer[messageSize == 1024 ? 1023 : messageSize] = '\0';

            std::string request(readBuffer);

            std::string reply = getRequestFromThread(request);
            
            write(ACCEPTEDCONNECTION, reply.c_str(), reply.length());

            close(ACCEPTEDCONNECTION);
        }

        close(SOCKET);
    }).detach();
}