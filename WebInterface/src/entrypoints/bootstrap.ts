import "../panels/lovelace/cards/register-cards";
import "../panels/lovelace/flow-app";
import { FlowStore } from "../data/flow-store";
import { FlowWebSocketConnection } from "../data/connection";
import { createFlow } from "../data/flow";
import { collectEntities, fetchLayout } from "../data/layout";
import { seedDemoStates } from "../data/demo-states";
import { ensureMdiRegistry } from "../common/icons/mdi-registry";

async function bootstrap(): Promise<void> {
  void ensureMdiRegistry();
  const store = new FlowStore();
  const wsProtocol = location.protocol === "https:" ? "wss:" : "ws:";
  const connection = new FlowWebSocketConnection(store, `${wsProtocol}//${location.host}/ws`);
  const flow = createFlow(store, connection);

  seedDemoStates(store);

  const lovelace = await fetchLayout();
  const addresses = collectEntities(lovelace);

  const app = document.querySelector("flow-app");
  if (!(app instanceof HTMLElement)) return;

  (app as import("../panels/lovelace/flow-app").FlowApp).flow = flow;
  (app as import("../panels/lovelace/flow-app").FlowApp).lovelace = lovelace;

  connection.trackAddresses(addresses);
  connection.connect();

  connection.subscribeStates(() => {
    app.requestUpdate();
  });
  connection.subscribeConnection(() => {
    app.requestUpdate();
  });

  // Re-query when tab becomes visible again
  document.addEventListener("visibilitychange", () => {
    if (document.visibilityState === "visible") {
      connection.query(addresses);
    }
  });
}

bootstrap().catch((err) => {
  // eslint-disable-next-line no-console
  console.error("Flow bootstrap failed", err);
});
