use std::collections::BTreeMap;
use std::net::{Ipv4Addr, SocketAddr, TcpStream, ToSocketAddrs, UdpSocket};
use std::time::{Duration, Instant};

use serde::Serialize;
use tauri::webview::Color;
use tauri::{AppHandle, Manager, Url, WebviewUrl, WebviewWindowBuilder};

const DEFAULT_HOST: &str = "127.0.0.1";
const DEFAULT_PORT: u16 = 8992;
const DISCOVERY_PORT: u16 = 8993;
const DISCOVER_REQUEST: &[u8] = br#"{"type":"flow_discover","v":1}"#;

struct LaunchDefaults {
    host: String,
    port: u16,
    host_from_arg: bool,
    port_from_arg: bool,
}

#[derive(Clone, Serialize)]
#[serde(rename_all = "camelCase")]
struct FlowEndpoint {
    host: String,
    port: u16,
    name: String,
    version: String,
    project: String,
}

fn clean_host(raw: &str) -> Option<String> {
    let host = raw.trim();
    if host.is_empty() || host.len() > 253 {
        return None;
    }
    let ok = host.chars().all(|c| {
        c.is_ascii_alphanumeric() || matches!(c, '.' | '-' | '_' | ':' | '[' | ']')
    });
    if ok {
        Some(host.to_string())
    } else {
        None
    }
}

fn parse_port(raw: &str) -> Option<u16> {
    let port = raw.trim().parse::<u16>().ok()?;
    if port == 0 {
        None
    } else {
        Some(port)
    }
}

fn parse_args() -> LaunchDefaults {
    let mut host = DEFAULT_HOST.to_string();
    let mut port = DEFAULT_PORT;
    let mut host_from_arg = false;
    let mut port_from_arg = false;
    let mut args = std::env::args().skip(1);
    while let Some(arg) = args.next() {
        if let Some(rest) = arg.strip_prefix("--host=") {
            if let Some(value) = clean_host(rest) {
                host = value;
                host_from_arg = true;
            }
        } else if arg == "--host" {
            if let Some(value) = args.next().as_deref().and_then(clean_host) {
                host = value;
                host_from_arg = true;
            }
        } else if let Some(rest) = arg.strip_prefix("--port=") {
            if let Some(value) = parse_port(rest) {
                port = value;
                port_from_arg = true;
            }
        } else if arg == "--port" {
            if let Some(value) = args.next().as_deref().and_then(parse_port) {
                port = value;
                port_from_arg = true;
            }
        }
    }
    LaunchDefaults {
        host,
        port,
        host_from_arg,
        port_from_arg,
    }
}

fn console_url(host: &str, port: u16) -> Result<Url, String> {
    let origin = if host.contains(':') && !host.starts_with('[') {
        format!("http://[{host}]:{port}/")
    } else {
        format!("http://{host}:{port}/")
    };
    Url::parse(&origin).map_err(|_| "地址无效".to_string())
}

fn endpoint_open(host: &str, port: u16) -> bool {
    let target = if host.contains(':') && !host.starts_with('[') {
        format!("[{host}]:{port}")
    } else {
        format!("{host}:{port}")
    };
    let Ok(mut addrs) = target.to_socket_addrs() else {
        return false;
    };
    let Some(addr) = addrs.next() else {
        return false;
    };
    TcpStream::connect_timeout(&addr, Duration::from_secs(2)).is_ok()
}

fn js_string(value: &str) -> String {
    let mut out = String::from("\"");
    for c in value.chars() {
        match c {
            '\\' => out.push_str("\\\\"),
            '"' => out.push_str("\\\""),
            _ => out.push(c),
        }
    }
    out.push('"');
    out
}

fn parse_discover_reply(bytes: &[u8]) -> Option<FlowEndpoint> {
    let value: serde_json::Value = serde_json::from_slice(bytes).ok()?;
    if value.get("type")?.as_str()? != "flow_discover_reply" {
        return None;
    }
    let host = clean_host(value.get("host")?.as_str()?)?;
    let port = value.get("port")?.as_u64()? as u16;
    if port == 0 {
        return None;
    }
    Some(FlowEndpoint {
        host,
        port,
        name: value
            .get("name")
            .and_then(|v| v.as_str())
            .unwrap_or("Flow")
            .to_string(),
        version: value
            .get("version")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
        project: value
            .get("project")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
    })
}

#[tauri::command]
fn discover_flows(timeout_ms: Option<u64>) -> Result<Vec<FlowEndpoint>, String> {
    let timeout = Duration::from_millis(timeout_ms.unwrap_or(900).clamp(200, 5000));
    let socket = UdpSocket::bind(SocketAddr::from((Ipv4Addr::UNSPECIFIED, 0)))
        .map_err(|e| format!("无法打开发现套接字：{e}"))?;
    socket
        .set_broadcast(true)
        .map_err(|e| format!("无法开启广播：{e}"))?;
    let _ = socket.set_read_timeout(Some(Duration::from_millis(120)));

    let targets = [
        SocketAddr::from((Ipv4Addr::BROADCAST, DISCOVERY_PORT)),
        SocketAddr::from((Ipv4Addr::LOCALHOST, DISCOVERY_PORT)),
    ];
    for target in targets {
        let _ = socket.send_to(DISCOVER_REQUEST, target);
    }

    let mut found: BTreeMap<String, FlowEndpoint> = BTreeMap::new();
    let deadline = Instant::now() + timeout;
    let mut buf = [0u8; 2048];
    while Instant::now() < deadline {
        match socket.recv_from(&mut buf) {
            Ok((n, _)) => {
                if let Some(endpoint) = parse_discover_reply(&buf[..n]) {
                    let key = format!("{}:{}", endpoint.host, endpoint.port);
                    found.insert(key, endpoint);
                }
            }
            Err(_) => continue,
        }
    }
    Ok(found.into_values().collect())
}

#[tauri::command]
fn connect_console(app: AppHandle, host: String, port: u16) -> Result<(), String> {
    let Some(host) = clean_host(&host) else {
        return Err("地址无效".to_string());
    };
    if port == 0 {
        return Err("端口无效".to_string());
    }
    if !endpoint_open(&host, port) {
        return Err(format!(
            "无法连接 {host}:{port}。请确认 Flow 已启动，并且地址、端口正确。"
        ));
    }
    let url = console_url(&host, port)?;
    let Some(window) = app.get_webview_window("main") else {
        return Err("窗口不存在".to_string());
    };
    window
        .navigate(url)
        .map_err(|_| "无法打开控制台页面".to_string())?;
    let title = format!("FlowViewer · {host}:{port}");
    let _ = window.set_title(&title);
    Ok(())
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    let defaults = parse_args();

    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![connect_console, discover_flows])
        .setup(move |app| {
            let window =
                WebviewWindowBuilder::new(app, "main", WebviewUrl::App("index.html".into()))
                    .title("FlowViewer")
                    .inner_size(1280.0, 800.0)
                    .min_inner_size(960.0, 640.0)
                    .center()
                    .background_color(Color(26, 26, 30, 255))
                    .on_navigation(|url| matches!(url.scheme(), "http" | "https"))
                    .build()?;

            let script = format!(
                "window.__FLOW_CONSOLE_DEFAULTS={{host:{},port:{},hostFromArg:{},portFromArg:{}}};if(window.applyConsoleDefaults)window.applyConsoleDefaults();",
                js_string(&defaults.host),
                defaults.port,
                defaults.host_from_arg,
                defaults.port_from_arg,
            );
            let handle = app.handle().clone();
            std::thread::spawn(move || {
                for _ in 0..30 {
                    if let Some(webview) = handle.get_webview_window("main") {
                        let _ = webview.eval(&script);
                    }
                    std::thread::sleep(Duration::from_millis(100));
                }
            });
            let _ = window;
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running FlowViewer");
}
