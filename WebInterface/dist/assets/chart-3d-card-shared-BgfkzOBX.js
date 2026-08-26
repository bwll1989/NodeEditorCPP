import{i as C,n,r as x,e as v,a as w,o as L,b as d,t as f,C as D,A as u,c as h}from"./main-3VGlwnuu.js";import{u as z,i as S,a as A,b as R,c as g,d as O,e as P}from"./echarts-DsuMhSbC.js";var E=Object.defineProperty,T=Object.getOwnPropertyDescriptor,a=(t,e,i,r)=>{for(var o=r>1?void 0:r?T(e,i):e,l=t.length-1,p;l>=0;l--)(p=t[l])&&(o=(r?p(e,i,o):p(o))||o);return r&&o&&E(e,i,o),o};z([A,R,g,O,P]);const $=25,k=40;function _(t){return!!t&&t.clientWidth>0&&t.clientHeight>0}const b="rgba(0, 0, 0, 0.2)",B="rgba(0, 0, 0, 0.54)",c="rgba(0, 0, 0, 0.6)",H="rgba(0, 0, 0, 0.08)";let s=class extends w{constructor(){super(...arguments),this.mode="line",this.points=[],this.bgColor="",this.lineWidth=3,this.lineColor="#03a9f4",this.symbolSize=8,this.pointColor="#03a9f4",this.xLabel="X",this.yLabel="Y",this.zLabel="Z",this.showAxes=!0,this.autoRotate=!1,this._viewAlpha=$,this._viewBeta=k,this._chart=null,this._resizeObserver=null,this._initRetries=0,this._pointerDragging=!1,this._pointerLastX=0,this._pointerLastY=0,this._pointerBoundDom=null,this._onChartPointerDown=t=>{if(t.button===0){this._pointerDragging=!0,this._pointerLastX=t.clientX,this._pointerLastY=t.clientY;try{t.currentTarget.setPointerCapture(t.pointerId)}catch{}t.preventDefault(),t.stopPropagation()}},this._onChartPointerMove=t=>{if(!this._pointerDragging)return;const e=t.clientX-this._pointerLastX,i=t.clientY-this._pointerLastY;e===0&&i===0||(this._pointerLastX=t.clientX,this._pointerLastY=t.clientY,this._viewBeta+=e*.4,this._viewAlpha=Math.max(-90,Math.min(90,this._viewAlpha+i*.4)),this._applyViewAngles(!1),t.preventDefault(),t.stopPropagation())},this._onChartPointerUp=t=>{if(this._pointerDragging){this._pointerDragging=!1;try{t.currentTarget.releasePointerCapture(t.pointerId)}catch{}t.stopPropagation()}}}disconnectedCallback(){this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._teardownChart(),super.disconnectedCallback()}firstUpdated(){this._setupResizeObserver(),this._scheduleEnsureChart()}_scheduleEnsureChart(){if(this._ensureChart()){this._initRetries=0;return}this._initRetries>=40||(this._initRetries+=1,this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._initRetryTimer=setTimeout(()=>{this._initRetryTimer=void 0,this._scheduleEnsureChart()},120))}updated(t){if(t.has("points")){this._renderChart(this._chart?"data":"full");return}(t.has("mode")||t.has("lineWidth")||t.has("lineColor")||t.has("symbolSize")||t.has("pointColor")||t.has("xLabel")||t.has("yLabel")||t.has("zLabel")||t.has("showAxes")||t.has("autoRotate")||t.has("bgColor"))&&this._renderChart("full")}_setupResizeObserver(){typeof ResizeObserver>"u"||(this._resizeObserver=new ResizeObserver(()=>this._resizeChart()),this._chartRoot&&this._resizeObserver.observe(this._chartRoot),this._chartHost&&this._resizeObserver.observe(this._chartHost),this._resizeObserver.observe(this))}_teardownChart(){var t,e;this._unbindPointerHandlers();try{(t=this._resizeObserver)==null||t.disconnect()}catch{}this._resizeObserver=null;try{(e=this._chart)==null||e.dispose()}catch{}this._chart=null}_showAxesEnabled(){return this.showAxes!==!1}_axisOptions(t){const e=t?{show:!0,lineStyle:{color:b}}:{show:!1},i=t?{show:!0,lineStyle:{color:b}}:{show:!1},r=t?{show:!0,color:B}:{show:!1};return{grid3D:{show:t,axisLine:e,axisTick:i,axisLabel:r,splitLine:t?{show:!0,lineStyle:{color:H}}:{show:!1},splitArea:{show:!1}},xAxis3D:{type:"value",name:t?this.xLabel||"X":"",show:t,axisLine:e,axisTick:i,axisLabel:r,nameTextStyle:{color:c}},yAxis3D:{type:"value",name:t?this.yLabel||"Y":"",show:t,axisLine:e,axisTick:i,axisLabel:r,nameTextStyle:{color:c}},zAxis3D:{type:"value",name:t?this.zLabel||"Z":"",show:t,axisLine:e,axisTick:i,axisLabel:r,nameTextStyle:{color:c}}}}_buildOption(t=!0){const e=Array.isArray(this.points)?this.points:[],i=this._showAxesEnabled(),r=this._axisOptions(i),o=this.mode==="scatter"?[{type:"scatter3D",data:e,silent:!0,symbolSize:Number(this.symbolSize)||8,itemStyle:{color:this.pointColor||"#03a9f4",opacity:.92},emphasis:{itemStyle:{color:"#ff9800"}}}]:[{type:"line3D",data:e,silent:!0,lineStyle:{width:Number(this.lineWidth)||3,color:this.lineColor||"#03a9f4",opacity:.95}}],l={projection:"perspective",autoRotate:!!this.autoRotate,autoRotateSpeed:8,distance:200,rotate:!0,zoom:!0,pan:!0,rotateSensitivity:1,zoomSensitivity:1,panSensitivity:1,...t?{alpha:this._viewAlpha,beta:this._viewBeta}:{}};return{backgroundColor:"transparent",tooltip:{},grid3D:{boxWidth:100,boxHeight:100,boxDepth:100,...r.grid3D,viewControl:l,light:{main:{intensity:1.1,shadow:!1},ambient:{intensity:.55}}},xAxis3D:r.xAxis3D,yAxis3D:r.yAxis3D,zAxis3D:r.zAxis3D,series:o}}_renderChart(t="data"){if(!this._chart)return;if(t==="full"){this._chart.setOption(this._buildOption(!0),{notMerge:!0});return}const e=Array.isArray(this.points)?this.points:[];this._chart.setOption({series:[{data:e}]})}_ensureChart(){const t=this._chartHost;return!t||!_(t)?!1:(this._chart||(this._chart=S(t),this._bindPointerHandlers()),this._renderChart("full"),!0)}_bindPointerHandlers(){var e,i;const t=(i=(e=this._chart)==null?void 0:e.getZr())==null?void 0:i.dom;!t||this._pointerBoundDom===t||(this._unbindPointerHandlers(),this._pointerBoundDom=t,t.addEventListener("pointerdown",this._onChartPointerDown),t.addEventListener("pointermove",this._onChartPointerMove),t.addEventListener("pointerup",this._onChartPointerUp),t.addEventListener("pointercancel",this._onChartPointerUp))}_unbindPointerHandlers(){if(!this._pointerBoundDom)return;const t=this._pointerBoundDom;t.removeEventListener("pointerdown",this._onChartPointerDown),t.removeEventListener("pointermove",this._onChartPointerMove),t.removeEventListener("pointerup",this._onChartPointerUp),t.removeEventListener("pointercancel",this._onChartPointerUp),this._pointerBoundDom=null}_applyViewAngles(t=!0){if(!this._chart){this._ensureChart();return}const e=this._showAxesEnabled(),i=this._axisOptions(e);try{this._chart.setOption({grid3D:{...i.grid3D,viewControl:{alpha:this._viewAlpha,beta:this._viewBeta,animation:t,animationDurationUpdate:t?280:0}},xAxis3D:i.xAxis3D,yAxis3D:i.yAxis3D,zAxis3D:i.zAxis3D})}catch{}}_resizeChart(){if(!_(this._chartHost)&&!_(this._chartRoot)){this._scheduleEnsureChart();return}if(!this._chart){this._ensureChart();return}try{this._chart.resize()}catch{}}render(){var e;const t=L({...this.bgColor?{backgroundColor:this.bgColor}:{}});return d`
      <div class="chart-root chart-interactive" style=${t}>
        <div class="chart-host chart-interactive"></div>
        <div class="chart-meta">${((e=this.points)==null?void 0:e.length)??0} pts</div>
      </div>
    `}};s.styles=C`
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
  `;a([n({type:String})],s.prototype,"mode",2);a([n({attribute:!1})],s.prototype,"points",2);a([n({type:String})],s.prototype,"bgColor",2);a([n({type:Number})],s.prototype,"lineWidth",2);a([n({type:String})],s.prototype,"lineColor",2);a([n({type:Number})],s.prototype,"symbolSize",2);a([n({type:String})],s.prototype,"pointColor",2);a([n({type:String})],s.prototype,"xLabel",2);a([n({type:String})],s.prototype,"yLabel",2);a([n({type:String})],s.prototype,"zLabel",2);a([n({type:Boolean})],s.prototype,"showAxes",2);a([n({type:Boolean})],s.prototype,"autoRotate",2);a([x()],s.prototype,"_viewAlpha",2);a([x()],s.prototype,"_viewBeta",2);a([v(".chart-host")],s.prototype,"_chartHost",2);a([v(".chart-root")],s.prototype,"_chartRoot",2);s=a([f("ha-chart-3d")],s);var U=Object.defineProperty,X=(t,e,i,r)=>{for(var o=void 0,l=t.length-1,p;l>=0;l--)(p=t[l])&&(o=p(e,i,o)||o);return o&&U(e,i,o),o};function y(t,e){if(t==null)return e;if(typeof t=="boolean")return t;if(typeof t=="number")return t!==0;if(typeof t=="string"){const i=t.trim().toLowerCase();if(i==="false"||i==="0"||i==="no"||i==="off")return!1;if(i==="true"||i==="1"||i==="yes"||i==="on")return!0}return!!t}function m(t){const e=Number((t==null?void 0:t.max_points)??h.max_points);return{bgColor:String((t==null?void 0:t.bg_color)??h.bg_color),maxPoints:Number.isFinite(e)&&e>0?Math.floor(e):h.max_points,lineWidth:Number((t==null?void 0:t.line_width)??h.line_width),lineColor:String((t==null?void 0:t.line_color)??h.line_color),symbolSize:Number((t==null?void 0:t.symbol_size)??h.symbol_size),pointColor:String((t==null?void 0:t.point_color)??h.point_color),xLabel:String((t==null?void 0:t.x_label)??h.x_label),yLabel:String((t==null?void 0:t.y_label)??h.y_label),zLabel:String((t==null?void 0:t.z_label)??h.z_label),showAxes:y(t==null?void 0:t.show_axes,h.show_axes),autoRotate:y(t==null?void 0:t.auto_rotate,h.auto_rotate)}}const M=C`
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
`;class N extends w{constructor(){super(...arguments),this._tracker=new D,this._pointsRevision=0}setConfig(e){var r;const i=(r=this._config)==null?void 0:r.entity;this._config=e,e.entity!==i&&this._tracker.resetForEntity(String(e.entity??"")),this._syncTracker(),this.requestUpdate()}connectedCallback(){super.connectedCallback(),this.classList.add("chart-interactive"),this._syncTracker()}disconnectedCallback(){this._tracker.dispose(),super.disconnectedCallback()}updated(e){e.has("flow")&&this._syncTracker()}_syncTracker(){var r;const e=(r=this._config)!=null&&r.entity?String(this._config.entity):void 0,i=m(this._config);this._tracker.bind(this.flow,e,i.maxPoints,()=>{this._pointsRevision+=1,this.requestUpdate()})}getGridOptions(){return{columns:12,rows:4,min_columns:6,min_rows:3}}renderChart3d(){if(!this._config)return u;const e=m(this._config);this._pointsRevision;const i=this._config.name?String(this._config.name):"";return d`
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
    `}}X([n({attribute:!1})],N.prototype,"flow");export{N as H,M as c};
