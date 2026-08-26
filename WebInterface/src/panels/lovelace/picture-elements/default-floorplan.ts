/** Offline floor-plan placeholder (HA picture-elements stub equivalent). */
export const DEFAULT_FLOORPLAN = `data:image/svg+xml,${encodeURIComponent(`
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 560">
  <rect width="900" height="560" fill="#ead9c2"/>
  <g fill="#f4ece1" stroke="#4c3828" stroke-width="7">
    <rect x="24" y="24" width="368" height="268"/>
    <rect x="392" y="24" width="484" height="268"/>
    <rect x="24" y="292" width="248" height="244"/>
    <rect x="272" y="292" width="278" height="244"/>
    <rect x="550" y="292" width="326" height="244"/>
  </g>
  <g fill="none" stroke="#4c3828" stroke-width="7">
    <rect x="24" y="24" width="852" height="512" rx="4"/>
  </g>
  <g fill="#6d5644" font-family="Segoe UI, sans-serif" font-size="22" text-anchor="middle">
    <text x="208" y="164">客厅</text>
    <text x="634" y="164">厨房</text>
    <text x="148" y="424">卧室</text>
    <text x="411" y="424">卫生间</text>
    <text x="713" y="424">阳台</text>
  </g>
</svg>
`)}`;
