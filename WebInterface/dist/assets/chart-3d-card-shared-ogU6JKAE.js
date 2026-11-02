import{i as y,n,r as m,e as x,a as C,o as v,b as d,t as w,C as L,A as u,c as h}from"./main-CIKDZczu.js";import{o as f,r as D}from"./chart-theme-D_IwiX_C.js";import{u as z,i as S,a as A,b as P,c as R,d as g,e as T}from"./echarts-DsuMhSbC.js";var O=Object.defineProperty,E=Object.getOwnPropertyDescriptor,a=(t,e,i,r)=>{for(var s=r>1?void 0:r?E(e,i):e,l=t.length-1,p;l>=0;l--)(p=t[l])&&(s=(r?p(e,i,s):p(s))||s);return r&&s&&O(e,i,s),s};z([A,P,R,g,T]);const $=25,k=40;function _(t){return!!t&&t.clientWidth>0&&t.clientHeight>0}let o=class extends C{constructor(){super(...arguments),this.mode="line",this.points=[],this.bgColor="",this.lineWidth=3,this.lineColor="#79ae31",this.symbolSize=8,this.pointColor="#79ae31",this.xLabel="X",this.yLabel="Y",this.zLabel="Z",this.showAxes=!0,this.autoRotate=!1,this._viewAlpha=$,this._viewBeta=k,this._chart=null,this._resizeObserver=null,this._initRetries=0,this._pointerDragging=!1,this._pointerLastX=0,this._pointerLastY=0,this._pointerBoundDom=null,this._onChartPointerDown=t=>{if(t.button===0){this._pointerDragging=!0,this._pointerLastX=t.clientX,this._pointerLastY=t.clientY;try{t.currentTarget.setPointerCapture(t.pointerId)}catch{}t.preventDefault(),t.stopPropagation()}},this._onChartPointerMove=t=>{if(!this._pointerDragging)return;const e=t.clientX-this._pointerLastX,i=t.clientY-this._pointerLastY;e===0&&i===0||(this._pointerLastX=t.clientX,this._pointerLastY=t.clientY,this._viewBeta+=e*.4,this._viewAlpha=Math.max(-90,Math.min(90,this._viewAlpha+i*.4)),this._applyViewAngles(!1),t.preventDefault(),t.stopPropagation())},this._onChartPointerUp=t=>{if(this._pointerDragging){this._pointerDragging=!1;try{t.currentTarget.releasePointerCapture(t.pointerId)}catch{}t.stopPropagation()}}}disconnectedCallback(){this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._teardownChart(),super.disconnectedCallback()}firstUpdated(){this._setupResizeObserver(),this._scheduleEnsureChart(),f(this,()=>this._renderChart(this._chart?"full":"data"))}_scheduleEnsureChart(){if(this._ensureChart()){this._initRetries=0;return}this._initRetries>=40||(this._initRetries+=1,this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._initRetryTimer=setTimeout(()=>{this._initRetryTimer=void 0,this._scheduleEnsureChart()},120))}updated(t){if(t.has("points")){this._renderChart(this._chart?"data":"full");return}(t.has("mode")||t.has("lineWidth")||t.has("lineColor")||t.has("symbolSize")||t.has("pointColor")||t.has("xLabel")||t.has("yLabel")||t.has("zLabel")||t.has("showAxes")||t.has("autoRotate")||t.has("bgColor"))&&this._renderChart("full")}_setupResizeObserver(){typeof ResizeObserver>"u"||(this._resizeObserver=new ResizeObserver(()=>this._resizeChart()),this._chartRoot&&this._resizeObserver.observe(this._chartRoot),this._chartHost&&this._resizeObserver.observe(this._chartHost),this._resizeObserver.observe(this))}_teardownChart(){var t,e;this._unbindPointerHandlers();try{(t=this._resizeObserver)==null||t.disconnect()}catch{}this._resizeObserver=null;try{(e=this._chart)==null||e.dispose()}catch{}this._chart=null}_showAxesEnabled(){return this.showAxes!==!1}_axisOptions(t){const e=D(this),i=t?{show:!0,lineStyle:{color:e.axisLine}}:{show:!1},r=t?{show:!0,lineStyle:{color:e.axisLine}}:{show:!1},s=t?{show:!0,color:e.axisLabel}:{show:!1},l=t?{show:!0,lineStyle:{color:e.splitLine}}:{show:!1};return{grid3D:{show:t,axisLine:i,axisTick:r,axisLabel:s,splitLine:l,splitArea:{show:!1}},xAxis3D:{type:"value",name:t?this.xLabel||"X":"",show:t,axisLine:i,axisTick:r,axisLabel:s,nameTextStyle:{color:e.axisName}},yAxis3D:{type:"value",name:t?this.yLabel||"Y":"",show:t,axisLine:i,axisTick:r,axisLabel:s,nameTextStyle:{color:e.axisName}},zAxis3D:{type:"value",name:t?this.zLabel||"Z":"",show:t,axisLine:i,axisTick:r,axisLabel:s,nameTextStyle:{color:e.axisName}}}}_buildOption(t=!0){const e=Array.isArray(this.points)?this.points:[],i=this._showAxesEnabled(),r=this._axisOptions(i),s=this.mode==="scatter"?[{type:"scatter3D",data:e,silent:!0,symbolSize:Number(this.symbolSize)||8,itemStyle:{color:this.pointColor||"#79ae31",opacity:.92},emphasis:{itemStyle:{color:"#ff9800"}}}]:[{type:"line3D",data:e,silent:!0,lineStyle:{width:Number(this.lineWidth)||3,color:this.lineColor||"#79ae31",opacity:.95}}],l={projection:"perspective",autoRotate:!!this.autoRotate,autoRotateSpeed:8,distance:200,rotate:!0,zoom:!0,pan:!0,rotateSensitivity:1,zoomSensitivity:1,panSensitivity:1,...t?{alpha:this._viewAlpha,beta:this._viewBeta}:{}};return{backgroundColor:"transparent",tooltip:{},grid3D:{boxWidth:100,boxHeight:100,boxDepth:100,...r.grid3D,viewControl:l,light:{main:{intensity:1.1,shadow:!1},ambient:{intensity:.55}}},xAxis3D:r.xAxis3D,yAxis3D:r.yAxis3D,zAxis3D:r.zAxis3D,series:s}}_renderChart(t="data"){if(!this._chart)return;if(t==="full"){this._chart.setOption(this._buildOption(!0),{notMerge:!0});return}const e=Array.isArray(this.points)?this.points:[];this._chart.setOption({series:[{data:e}]})}_ensureChart(){const t=this._chartHost;return!t||!_(t)?!1:(this._chart||(this._chart=S(t),this._bindPointerHandlers()),this._renderChart("full"),!0)}_bindPointerHandlers(){var e,i;const t=(i=(e=this._chart)==null?void 0:e.getZr())==null?void 0:i.dom;!t||this._pointerBoundDom===t||(this._unbindPointerHandlers(),this._pointerBoundDom=t,t.addEventListener("pointerdown",this._onChartPointerDown),t.addEventListener("pointermove",this._onChartPointerMove),t.addEventListener("pointerup",this._onChartPointerUp),t.addEventListener("pointercancel",this._onChartPointerUp))}_unbindPointerHandlers(){if(!this._pointerBoundDom)return;const t=this._pointerBoundDom;t.removeEventListener("pointerdown",this._onChartPointerDown),t.removeEventListener("pointermove",this._onChartPointerMove),t.removeEventListener("pointerup",this._onChartPointerUp),t.removeEventListener("pointercancel",this._onChartPointerUp),this._pointerBoundDom=null}_applyViewAngles(t=!0){if(!this._chart){this._ensureChart();return}const e=this._showAxesEnabled(),i=this._axisOptions(e);try{this._chart.setOption({grid3D:{...i.grid3D,viewControl:{alpha:this._viewAlpha,beta:this._viewBeta,animation:t,animationDurationUpdate:t?280:0}},xAxis3D:i.xAxis3D,yAxis3D:i.yAxis3D,zAxis3D:i.zAxis3D})}catch{}}_resizeChart(){if(!_(this._chartHost)&&!_(this._chartRoot)){this._scheduleEnsureChart();return}if(!this._chart){this._ensureChart();return}try{this._chart.resize()}catch{}}render(){var e;const t=v({...this.bgColor?{backgroundColor:this.bgColor}:{}});return d`
      <div class="chart-root chart-interactive" style=${t}>
        <div class="chart-host chart-interactive"></div>
        <div class="chart-meta">${((e=this.points)==null?void 0:e.length)??0} pts</div>
      </div>
    `}};o.styles=y`
    :host {
      display: block;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 180px;
    }
    .chart-root {
      position: relative;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 180px;
      box-sizing: border-box;
      overflow: hidden;
      background: var(--card-background-color, #fff);
      border-radius: inherit;
    }
    .chart-host {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
      touch-action: none;
      cursor: grab;
    }
    .chart-host:active {
      cursor: grabbing;
    }
    .chart-meta {
      position: absolute;
      right: 8px;
      bottom: 6px;
      color: var(--secondary-text-color, rgba(0, 0, 0, 0.6));
      font-size: 11px;
      pointer-events: none;
      z-index: 2;
    }
  `;a([n({type:String})],o.prototype,"mode",2);a([n({attribute:!1})],o.prototype,"points",2);a([n({type:String})],o.prototype,"bgColor",2);a([n({type:Number})],o.prototype,"lineWidth",2);a([n({type:String})],o.prototype,"lineColor",2);a([n({type:Number})],o.prototype,"symbolSize",2);a([n({type:String})],o.prototype,"pointColor",2);a([n({type:String})],o.prototype,"xLabel",2);a([n({type:String})],o.prototype,"yLabel",2);a([n({type:String})],o.prototype,"zLabel",2);a([n({type:Boolean})],o.prototype,"showAxes",2);a([n({type:Boolean})],o.prototype,"autoRotate",2);a([m()],o.prototype,"_viewAlpha",2);a([m()],o.prototype,"_viewBeta",2);a([x(".chart-host")],o.prototype,"_chartHost",2);a([x(".chart-root")],o.prototype,"_chartRoot",2);o=a([w("ha-chart-3d")],o);var B=Object.defineProperty,H=(t,e,i,r)=>{for(var s=void 0,l=t.length-1,p;l>=0;l--)(p=t[l])&&(s=p(e,i,s)||s);return s&&B(e,i,s),s};function c(t,e){if(t==null)return e;if(typeof t=="boolean")return t;if(typeof t=="number")return t!==0;if(typeof t=="string"){const i=t.trim().toLowerCase();if(i==="false"||i==="0"||i==="no"||i==="off")return!1;if(i==="true"||i==="1"||i==="yes"||i==="on")return!0}return!!t}function b(t){const e=Number((t==null?void 0:t.max_points)??h.max_points);return{bgColor:String((t==null?void 0:t.bg_color)??h.bg_color),maxPoints:Number.isFinite(e)&&e>0?Math.floor(e):h.max_points,lineWidth:Number((t==null?void 0:t.line_width)??h.line_width),lineColor:String((t==null?void 0:t.line_color)??h.line_color),symbolSize:Number((t==null?void 0:t.symbol_size)??h.symbol_size),pointColor:String((t==null?void 0:t.point_color)??h.point_color),xLabel:String((t==null?void 0:t.x_label)??h.x_label),yLabel:String((t==null?void 0:t.y_label)??h.y_label),zLabel:String((t==null?void 0:t.z_label)??h.z_label),showAxes:c(t==null?void 0:t.show_axes,h.show_axes),autoRotate:c(t==null?void 0:t.auto_rotate,h.auto_rotate)}}const Y=y`
  :host {
    display: block;
    height: 100%;
    min-height: 180px;
  }
  ha-card {
    height: 100%;
    display: flex;
    flex-direction: column;
  }
  .card-header {
    margin: 0;
    padding: 12px 16px 0;
    font-size: 16px;
    font-weight: 500;
    color: var(--primary-text-color);
  }
  ha-chart-3d {
    display: block;
    flex: 1 1 auto;
    width: 100%;
    min-height: 180px;
    touch-action: none;
  }
`;class U extends C{constructor(){super(...arguments),this._tracker=new L,this._pointsRevision=0}setConfig(e){var r;const i=(r=this._config)==null?void 0:r.entity;this._config=e,e.entity!==i&&this._tracker.resetForEntity(String(e.entity??"")),this._syncTracker(),this.requestUpdate()}connectedCallback(){super.connectedCallback(),this.classList.add("chart-interactive"),this._syncTracker()}disconnectedCallback(){this._tracker.dispose(),super.disconnectedCallback()}updated(e){e.has("flow")&&this._syncTracker()}_syncTracker(){var r;const e=(r=this._config)!=null&&r.entity?String(this._config.entity):void 0,i=b(this._config);this._tracker.bind(this.flow,e,i.maxPoints,()=>{this._pointsRevision+=1,this.requestUpdate()})}getGridOptions(){return{columns:12,rows:4,min_columns:6,min_rows:3}}renderChart3d(){if(!this._config)return u;const e=b(this._config);this._pointsRevision;const i=this._config.name?String(this._config.name):"";return d`
      <ha-card>
        ${i?d`<h1 class="card-header">${i}</h1>`:u}
        <ha-chart-3d
          .mode=${this.chartMode}
          .points=${this._tracker.points}
          .bgColor=${e.bgColor}
          .lineWidth=${e.lineWidth}
          .lineColor=${e.lineColor}
          .symbolSize=${e.symbolSize}
          .pointColor=${e.pointColor}
          .xLabel=${e.xLabel}
          .yLabel=${e.yLabel}
          .zLabel=${e.zLabel}
          .showAxes=${e.showAxes}
          .autoRotate=${e.autoRotate}
        ></ha-chart-3d>
      </ha-card>
    `}}H([n({attribute:!1})],U.prototype,"flow");export{U as H,Y as c};
