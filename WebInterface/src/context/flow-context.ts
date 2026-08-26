import { createContext } from "@lit/context";
import type { Flow } from "../types";

export const flowContext = createContext<Flow>("flow");
