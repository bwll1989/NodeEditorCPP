import{d as w,i as m,n as l,e as x,a as C,o as f,b as p,t as S,f as v,A as d,g as c}from"./main-CIKDZczu.js";import{o as L,r as T}from"./chart-theme-D_IwiX_C.js";import{u as k,a as z,f as A,b as $,g as O,h as R,j as g,i as E}from"./echarts-DsuMhSbC.js";function u(t){if(!Array.isArray(t))return[];const e=[];for(const r of t){if(!r||typeof r!="object")continue;const i=r,s=String(i.entity??"").trim();s&&e.push({entity:s,...i.name?{name:String(i.name)}:{},...i.color?{color:String(i.color)}:{}})}return e}function N(t){if(!t)return 0;const e=t.state;return typeof e=="boolean"?e?1:0:w(e)}function G(t,e){var a,o;const r=(a=t.name)==null?void 0:a.trim();if(r)return r;const i=(o=e==null?void 0:e.attributes)==null?void 0:o.friendly_name;return typeof i=="string"&&i.trim()?i.trim():t.entity.split("/").filter(Boolean).pop()??t.entity}class H{constructor(){this.bars=[],this._configs=[],this._defaultColor="#79ae31",this._unsubs=[]}bind(e,r,i,s){if(this.dispose(),this._flow=e,this._configs=r,this._defaultColor=i||"#79ae31",this._onChange=s,!e){this._rebuild();return}r.forEach(a=>{const o=a.entity.trim();o&&this._unsubs.push(e.subscribeEntity(o,()=>{this._rebuild()}))}),this._rebuild()}dispose(){this._unsubs.forEach(e=>e()),this._unsubs=[],this._configs=[],this._flow=void 0,this._onChange=void 0,this.bars=[]}_rebuild(){var r;const e=this._flow;this.bars=this._configs.map(i=>{var o;const s=i.entity.trim(),a=e==null?void 0:e.states[s];return{name:G(i,a),value:N(a),color:((o=i.color)==null?void 0:o.trim())||this._defaultColor}}),(r=this._onChange)==null||r.call(this)}}var P=Object.defineProperty,B=Object.getOwnPropertyDescriptor,n=(t,e,r,i)=>{for(var s=i>1?void 0:i?B(e,r):e,a=t.length-1,o;a>=0;a--)(o=t[a])&&(s=(i?o(e,r,s):o(s))||s);return i&&s&&P(e,r,s),s};k([z,A,$,O,R,g]);function b(t){return!!t&&t.clientWidth>0&&t.clientHeight>0}let h=class extends C{constructor(){super(...arguments),this.mode="line",this.points=[],this.bars=[],this.bgColor="",this.lineWidth=2,this.lineColor="#79ae31",this.symbolSize=8,this.pointColor="#79ae31",this.barColor="#79ae31",this.xLabel="X",this.yLabel="Y",this.showAxes=!0,this.showGrid=!0,this._chart=null,this._resizeObserver=null,this._initRetries=0}disconnectedCallback(){this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._teardownChart(),super.disconnectedCallback()}firstUpdated(){this._setupResizeObserver(),this._scheduleEnsureChart(),L(this,()=>this._renderChart(this._chart?"full":"data"))}_scheduleEnsureChart(){if(this._ensureChart()){this._initRetries=0;return}this._initRetries>=40||(this._initRetries+=1,this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._initRetryTimer=setTimeout(()=>{this._initRetryTimer=void 0,this._scheduleEnsureChart()},120))}updated(t){if(t.has("points")||t.has("bars")){this._renderChart(this._chart?"data":"full");return}(t.has("mode")||t.has("lineWidth")||t.has("lineColor")||t.has("symbolSize")||t.has("pointColor")||t.has("barColor")||t.has("xLabel")||t.has("yLabel")||t.has("showAxes")||t.has("showGrid")||t.has("bgColor"))&&this._renderChart("full")}_setupResizeObserver(){typeof ResizeObserver>"u"||(this._resizeObserver=new ResizeObserver(()=>this._resizeChart()),this._chartRoot&&this._resizeObserver.observe(this._chartRoot),this._chartHost&&this._resizeObserver.observe(this._chartHost),this._resizeObserver.observe(this))}_teardownChart(){var t,e;try{(t=this._resizeObserver)==null||t.disconnect()}catch{}this._resizeObserver=null;try{(e=this._chart)==null||e.dispose()}catch{}this._chart=null}_showAxesEnabled(){return this.showAxes!==!1}_showGridEnabled(){return this.showGrid!==!1}_buildOption(){const t=this._showAxesEnabled(),e=this._showGridEnabled(),r=T(this);if(this.mode==="bar"){const a=Array.isArray(this.bars)?this.bars:[];return{backgroundColor:"transparent",tooltip:{trigger:"axis"},grid:{left:8,right:8,top:24,bottom:8,containLabel:!0},xAxis:{type:"category",data:a.map(o=>o.name),name:t&&this.xLabel||"",show:t,axisLine:{show:t,lineStyle:{color:r.axisLine}},axisTick:{show:t,lineStyle:{color:r.axisLine}},axisLabel:{show:t,color:r.axisLabel},nameTextStyle:{color:r.axisName}},yAxis:{type:"value",name:t?this.yLabel||"Y":"",show:t,axisLine:{show:t,lineStyle:{color:r.axisLine}},axisTick:{show:t,lineStyle:{color:r.axisLine}},axisLabel:{show:t,color:r.axisLabel},splitLine:{show:e,lineStyle:{color:r.splitLine}},nameTextStyle:{color:r.axisName}},series:[{type:"bar",data:a.map(o=>({value:o.value,itemStyle:{color:o.color||this.barColor||"#79ae31",borderRadius:[4,4,0,0]}}))}]}}const i=Array.isArray(this.points)?this.points:[],s=this.mode==="scatter"?[{type:"scatter",data:i,symbolSize:Number(this.symbolSize)||8,itemStyle:{color:this.pointColor||"#79ae31",opacity:.92}}]:[{type:"line",data:i,showSymbol:!1,lineStyle:{width:Number(this.lineWidth)||2,color:this.lineColor||"#79ae31"}}];return{backgroundColor:"transparent",tooltip:{trigger:"axis"},grid:{left:8,right:8,top:24,bottom:8,containLabel:!0},xAxis:{type:"value",name:t?this.xLabel||"X":"",show:t,axisLine:{show:t,lineStyle:{color:r.axisLine}},axisTick:{show:t,lineStyle:{color:r.axisLine}},axisLabel:{show:t,color:r.axisLabel},splitLine:{show:e,lineStyle:{color:r.splitLine}},nameTextStyle:{color:r.axisName}},yAxis:{type:"value",name:t?this.yLabel||"Y":"",show:t,axisLine:{show:t,lineStyle:{color:r.axisLine}},axisTick:{show:t,lineStyle:{color:r.axisLine}},axisLabel:{show:t,color:r.axisLabel},splitLine:{show:e,lineStyle:{color:r.splitLine}},nameTextStyle:{color:r.axisName}},series:s}}_renderChart(t="data"){if(!this._chart)return;if(t==="full"){this._chart.setOption(this._buildOption(),{notMerge:!0});return}if(this.mode==="bar"){const r=Array.isArray(this.bars)?this.bars:[];this._chart.setOption({xAxis:{data:r.map(i=>i.name)},series:[{data:r.map(i=>({value:i.value,itemStyle:{color:i.color||this.barColor||"#79ae31",borderRadius:[4,4,0,0]}}))}]});return}const e=Array.isArray(this.points)?this.points:[];this._chart.setOption({series:[{data:e}]})}_ensureChart(){const t=this._chartHost;return!t||!b(t)?!1:(this._chart||(this._chart=E(t)),this._renderChart("full"),!0)}_resizeChart(){if(!b(this._chartHost)&&!b(this._chartRoot)){this._scheduleEnsureChart();return}if(!this._chart){this._ensureChart();return}try{this._chart.resize()}catch{}}_pointCount(){var t,e;return this.mode==="bar"?((t=this.bars)==null?void 0:t.length)??0:((e=this.points)==null?void 0:e.length)??0}render(){const t=f({...this.bgColor?{backgroundColor:this.bgColor}:{}}),e=this.mode==="bar"?"bars":"pts";return p`
      <div class="chart-root chart-interactive" style=${t}>
        <div class="chart-host chart-interactive"></div>
        <div class="chart-meta">${this._pointCount()} ${e}</div>
      </div>
    `}};h.styles=m`
    :host {
      display: block;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 160px;
    }
    .chart-root {
      position: relative;
      width: 100%;
      height: 100%;
      min-width: 0;
      min-height: 160px;
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
  `;n([l({type:String})],h.prototype,"mode",2);n([l({attribute:!1})],h.prototype,"points",2);n([l({attribute:!1})],h.prototype,"bars",2);n([l({type:String})],h.prototype,"bgColor",2);n([l({type:Number})],h.prototype,"lineWidth",2);n([l({type:String})],h.prototype,"lineColor",2);n([l({type:Number})],h.prototype,"symbolSize",2);n([l({type:String})],h.prototype,"pointColor",2);n([l({type:String})],h.prototype,"barColor",2);n([l({type:String})],h.prototype,"xLabel",2);n([l({type:String})],h.prototype,"yLabel",2);n([l({type:Boolean})],h.prototype,"showAxes",2);n([l({type:Boolean})],h.prototype,"showGrid",2);n([x(".chart-host")],h.prototype,"_chartHost",2);n([x(".chart-root")],h.prototype,"_chartRoot",2);h=n([S("ha-chart-2d")],h);var W=Object.defineProperty,M=(t,e,r,i)=>{for(var s=void 0,a=t.length-1,o;a>=0;a--)(o=t[a])&&(s=o(e,r,s)||s);return s&&W(e,r,s),s};function _(t,e){if(t==null)return e;if(typeof t=="boolean")return t;if(typeof t=="number")return t!==0;if(typeof t=="string"){const r=t.trim().toLowerCase();if(r==="false"||r==="0"||r==="no"||r==="off")return!1;if(r==="true"||r==="1"||r==="yes"||r==="on")return!0}return!!t}function y(t){const e=Number((t==null?void 0:t.max_points)??c.max_points);return{bgColor:String((t==null?void 0:t.bg_color)??c.bg_color),maxPoints:Number.isFinite(e)&&e>0?Math.floor(e):c.max_points,lineWidth:Number((t==null?void 0:t.line_width)??c.line_width),lineColor:String((t==null?void 0:t.line_color)??c.line_color),symbolSize:Number((t==null?void 0:t.symbol_size)??c.symbol_size),pointColor:String((t==null?void 0:t.point_color)??c.point_color),barColor:String((t==null?void 0:t.bar_color)??c.bar_color),xLabel:String((t==null?void 0:t.x_label)??c.x_label),yLabel:String((t==null?void 0:t.y_label)??c.y_label),showAxes:_(t==null?void 0:t.show_axes,c.show_axes),showGrid:_(t==null?void 0:t.show_grid,c.show_grid)}}const F=m`
  :host {
    display: block;
    height: 100%;
    min-height: 160px;
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
  ha-chart-2d {
    display: block;
    flex: 1 1 auto;
    width: 100%;
    min-height: 160px;
  }
`;class j extends C{constructor(){super(...arguments),this._pointTracker=new v,this._barTracker=new H,this._dataRevision=0}_configTrackKey(e){return this.chartMode==="bar"?JSON.stringify(u(e==null?void 0:e.entities)):String((e==null?void 0:e.entity)??"")}setConfig(e){const r=this._configTrackKey(this._config);this._config=e,this._configTrackKey(e)!==r&&(this.chartMode==="bar"?this._barTracker.dispose():this._pointTracker.resetForEntity(String(e.entity??""))),this._syncTracker(),this.requestUpdate()}connectedCallback(){super.connectedCallback(),this.classList.add("chart-interactive"),this._syncTracker()}disconnectedCallback(){this._pointTracker.dispose(),this._barTracker.dispose(),super.disconnectedCallback()}updated(e){e.has("flow")&&this._syncTracker()}_syncTracker(){var s,a;const e=(s=this._config)!=null&&s.entity?String(this._config.entity):void 0,r=y(this._config),i=()=>{this._dataRevision+=1,this.requestUpdate()};if(this.chartMode==="bar"){this._pointTracker.dispose();const o=u((a=this._config)==null?void 0:a.entities);this._barTracker.bind(this.flow,o,r.barColor,i);return}this._barTracker.dispose(),this._pointTracker.bind(this.flow,e,r.maxPoints,i)}getGridOptions(){return{columns:12,rows:4,min_columns:6,min_rows:3}}renderChart2d(){if(!this._config)return d;const e=y(this._config);this._dataRevision;const r=this._config.name?String(this._config.name):"";return p`
      <ha-card>
        ${r?p`<h1 class="card-header">${r}</h1>`:d}
        <ha-chart-2d
          .mode=${this.chartMode}
          .points=${this._pointTracker.points}
          .bars=${this._barTracker.bars}
          .bgColor=${e.bgColor}
          .lineWidth=${e.lineWidth}
          .lineColor=${e.lineColor}
          .symbolSize=${e.symbolSize}
          .pointColor=${e.pointColor}
          .barColor=${e.barColor}
          .xLabel=${e.xLabel}
          .yLabel=${e.yLabel}
          .showAxes=${e.showAxes}
          .showGrid=${e.showGrid}
        ></ha-chart-2d>
      </ha-card>
    `}}M([l({attribute:!1})],j.prototype,"flow");export{j as H,F as c};
