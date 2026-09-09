import{d as L,i as w,n as l,e as S,a as v,o as T,b as y,t as k,f as A,A as m,g as c}from"./main-DMBf8VcT.js";import{u as O,a as z,f as R,b as $,g,h as E,j as N,i as G}from"./echarts-DsuMhSbC.js";function C(t){if(!Array.isArray(t))return[];const r=[];for(const e of t){if(!e||typeof e!="object")continue;const s=e,i=String(s.entity??"").trim();i&&r.push({entity:i,...s.name?{name:String(s.name)}:{},...s.color?{color:String(s.color)}:{}})}return r}function P(t){if(!t)return 0;const r=t.state;return typeof r=="boolean"?r?1:0:L(r)}function B(t,r){var o,h;const e=(o=t.name)==null?void 0:o.trim();if(e)return e;const s=(h=r==null?void 0:r.attributes)==null?void 0:h.friendly_name;return typeof s=="string"&&s.trim()?s.trim():t.entity.split("/").filter(Boolean).pop()??t.entity}class H{constructor(){this.bars=[],this._configs=[],this._defaultColor="#03a9f4",this._unsubs=[]}bind(r,e,s,i){if(this.dispose(),this._flow=r,this._configs=e,this._defaultColor=s||"#03a9f4",this._onChange=i,!r){this._rebuild();return}e.forEach(o=>{const h=o.entity.trim();h&&this._unsubs.push(r.subscribeEntity(h,()=>{this._rebuild()}))}),this._rebuild()}dispose(){this._unsubs.forEach(r=>r()),this._unsubs=[],this._configs=[],this._flow=void 0,this._onChange=void 0,this.bars=[]}_rebuild(){var e;const r=this._flow;this.bars=this._configs.map(s=>{var h;const i=s.entity.trim(),o=r==null?void 0:r.states[i];return{name:B(s,o),value:P(o),color:((h=s.color)==null?void 0:h.trim())||this._defaultColor}}),(e=this._onChange)==null||e.call(this)}}var M=Object.defineProperty,W=Object.getOwnPropertyDescriptor,n=(t,r,e,s)=>{for(var i=s>1?void 0:s?W(r,e):r,o=t.length-1,h;o>=0;o--)(h=t[o])&&(i=(s?h(r,e,i):h(i))||i);return s&&i&&M(r,e,i),i};O([z,R,$,g,E,N]);function _(t){return!!t&&t.clientWidth>0&&t.clientHeight>0}const b="rgba(0, 0, 0, 0.2)",p="rgba(0, 0, 0, 0.54)",d="rgba(0, 0, 0, 0.6)",u="rgba(0, 0, 0, 0.08)";let a=class extends v{constructor(){super(...arguments),this.mode="line",this.points=[],this.bars=[],this.bgColor="",this.lineWidth=2,this.lineColor="#03a9f4",this.symbolSize=8,this.pointColor="#03a9f4",this.barColor="#03a9f4",this.xLabel="X",this.yLabel="Y",this.showAxes=!0,this.showGrid=!0,this._chart=null,this._resizeObserver=null,this._initRetries=0}disconnectedCallback(){this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._teardownChart(),super.disconnectedCallback()}firstUpdated(){this._setupResizeObserver(),this._scheduleEnsureChart()}_scheduleEnsureChart(){if(this._ensureChart()){this._initRetries=0;return}this._initRetries>=40||(this._initRetries+=1,this._initRetryTimer&&clearTimeout(this._initRetryTimer),this._initRetryTimer=setTimeout(()=>{this._initRetryTimer=void 0,this._scheduleEnsureChart()},120))}updated(t){if(t.has("points")||t.has("bars")){this._renderChart(this._chart?"data":"full");return}(t.has("mode")||t.has("lineWidth")||t.has("lineColor")||t.has("symbolSize")||t.has("pointColor")||t.has("barColor")||t.has("xLabel")||t.has("yLabel")||t.has("showAxes")||t.has("showGrid")||t.has("bgColor"))&&this._renderChart("full")}_setupResizeObserver(){typeof ResizeObserver>"u"||(this._resizeObserver=new ResizeObserver(()=>this._resizeChart()),this._chartRoot&&this._resizeObserver.observe(this._chartRoot),this._chartHost&&this._resizeObserver.observe(this._chartHost),this._resizeObserver.observe(this))}_teardownChart(){var t,r;try{(t=this._resizeObserver)==null||t.disconnect()}catch{}this._resizeObserver=null;try{(r=this._chart)==null||r.dispose()}catch{}this._chart=null}_showAxesEnabled(){return this.showAxes!==!1}_showGridEnabled(){return this.showGrid!==!1}_buildOption(){const t=this._showAxesEnabled(),r=this._showGridEnabled();if(this.mode==="bar"){const i=Array.isArray(this.bars)?this.bars:[];return{backgroundColor:"transparent",tooltip:{trigger:"axis"},grid:{left:8,right:8,top:24,bottom:8,containLabel:!0},xAxis:{type:"category",data:i.map(o=>o.name),name:t&&this.xLabel||"",show:t,axisLine:{show:t,lineStyle:{color:b}},axisTick:{show:t,lineStyle:{color:b}},axisLabel:{show:t,color:p},nameTextStyle:{color:d}},yAxis:{type:"value",name:t?this.yLabel||"Y":"",show:t,axisLine:{show:t,lineStyle:{color:b}},axisTick:{show:t,lineStyle:{color:b}},axisLabel:{show:t,color:p},splitLine:{show:r,lineStyle:{color:u}},nameTextStyle:{color:d}},series:[{type:"bar",data:i.map(o=>({value:o.value,itemStyle:{color:o.color||this.barColor||"#03a9f4",borderRadius:[4,4,0,0]}}))}]}}const e=Array.isArray(this.points)?this.points:[],s=this.mode==="scatter"?[{type:"scatter",data:e,symbolSize:Number(this.symbolSize)||8,itemStyle:{color:this.pointColor||"#03a9f4",opacity:.92}}]:[{type:"line",data:e,showSymbol:!1,lineStyle:{width:Number(this.lineWidth)||2,color:this.lineColor||"#03a9f4"}}];return{backgroundColor:"transparent",tooltip:{trigger:"axis"},grid:{left:8,right:8,top:24,bottom:8,containLabel:!0},xAxis:{type:"value",name:t?this.xLabel||"X":"",show:t,axisLine:{show:t,lineStyle:{color:b}},axisTick:{show:t,lineStyle:{color:b}},axisLabel:{show:t,color:p},splitLine:{show:r,lineStyle:{color:u}},nameTextStyle:{color:d}},yAxis:{type:"value",name:t?this.yLabel||"Y":"",show:t,axisLine:{show:t,lineStyle:{color:b}},axisTick:{show:t,lineStyle:{color:b}},axisLabel:{show:t,color:p},splitLine:{show:r,lineStyle:{color:u}},nameTextStyle:{color:d}},series:s}}_renderChart(t="data"){if(!this._chart)return;if(t==="full"){this._chart.setOption(this._buildOption(),{notMerge:!0});return}if(this.mode==="bar"){const e=Array.isArray(this.bars)?this.bars:[];this._chart.setOption({xAxis:{data:e.map(s=>s.name)},series:[{data:e.map(s=>({value:s.value,itemStyle:{color:s.color||this.barColor||"#03a9f4",borderRadius:[4,4,0,0]}}))}]});return}const r=Array.isArray(this.points)?this.points:[];this._chart.setOption({series:[{data:r}]})}_ensureChart(){const t=this._chartHost;return!t||!_(t)?!1:(this._chart||(this._chart=G(t)),this._renderChart("full"),!0)}_resizeChart(){if(!_(this._chartHost)&&!_(this._chartRoot)){this._scheduleEnsureChart();return}if(!this._chart){this._ensureChart();return}try{this._chart.resize()}catch{}}_pointCount(){var t,r;return this.mode==="bar"?((t=this.bars)==null?void 0:t.length)??0:((r=this.points)==null?void 0:r.length)??0}render(){const t=T({...this.bgColor?{backgroundColor:this.bgColor}:{}}),r=this.mode==="bar"?"bars":"pts";return y`
      <div class="chart-root chart-interactive" style=${t}>
        <div class="chart-host chart-interactive"></div>
        <div class="chart-meta">${this._pointCount()} ${r}</div>
      </div>
    `}};a.styles=w`
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
  `;n([l({type:String})],a.prototype,"mode",2);n([l({attribute:!1})],a.prototype,"points",2);n([l({attribute:!1})],a.prototype,"bars",2);n([l({type:String})],a.prototype,"bgColor",2);n([l({type:Number})],a.prototype,"lineWidth",2);n([l({type:String})],a.prototype,"lineColor",2);n([l({type:Number})],a.prototype,"symbolSize",2);n([l({type:String})],a.prototype,"pointColor",2);n([l({type:String})],a.prototype,"barColor",2);n([l({type:String})],a.prototype,"xLabel",2);n([l({type:String})],a.prototype,"yLabel",2);n([l({type:Boolean})],a.prototype,"showAxes",2);n([l({type:Boolean})],a.prototype,"showGrid",2);n([S(".chart-host")],a.prototype,"_chartHost",2);n([S(".chart-root")],a.prototype,"_chartRoot",2);a=n([k("ha-chart-2d")],a);var I=Object.defineProperty,j=(t,r,e,s)=>{for(var i=void 0,o=t.length-1,h;o>=0;o--)(h=t[o])&&(i=h(r,e,i)||i);return i&&I(r,e,i),i};function x(t,r){if(t==null)return r;if(typeof t=="boolean")return t;if(typeof t=="number")return t!==0;if(typeof t=="string"){const e=t.trim().toLowerCase();if(e==="false"||e==="0"||e==="no"||e==="off")return!1;if(e==="true"||e==="1"||e==="yes"||e==="on")return!0}return!!t}function f(t){const r=Number((t==null?void 0:t.max_points)??c.max_points);return{bgColor:String((t==null?void 0:t.bg_color)??c.bg_color),maxPoints:Number.isFinite(r)&&r>0?Math.floor(r):c.max_points,lineWidth:Number((t==null?void 0:t.line_width)??c.line_width),lineColor:String((t==null?void 0:t.line_color)??c.line_color),symbolSize:Number((t==null?void 0:t.symbol_size)??c.symbol_size),pointColor:String((t==null?void 0:t.point_color)??c.point_color),barColor:String((t==null?void 0:t.bar_color)??c.bar_color),xLabel:String((t==null?void 0:t.x_label)??c.x_label),yLabel:String((t==null?void 0:t.y_label)??c.y_label),showAxes:x(t==null?void 0:t.show_axes,c.show_axes),showGrid:x(t==null?void 0:t.show_grid,c.show_grid)}}const U=w`
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
`;class D extends v{constructor(){super(...arguments),this._pointTracker=new A,this._barTracker=new H,this._dataRevision=0}_configTrackKey(r){return this.chartMode==="bar"?JSON.stringify(C(r==null?void 0:r.entities)):String((r==null?void 0:r.entity)??"")}setConfig(r){const e=this._configTrackKey(this._config);this._config=r,this._configTrackKey(r)!==e&&(this.chartMode==="bar"?this._barTracker.dispose():this._pointTracker.resetForEntity(String(r.entity??""))),this._syncTracker(),this.requestUpdate()}connectedCallback(){super.connectedCallback(),this.classList.add("chart-interactive"),this._syncTracker()}disconnectedCallback(){this._pointTracker.dispose(),this._barTracker.dispose(),super.disconnectedCallback()}updated(r){r.has("flow")&&this._syncTracker()}_syncTracker(){var i,o;const r=(i=this._config)!=null&&i.entity?String(this._config.entity):void 0,e=f(this._config),s=()=>{this._dataRevision+=1,this.requestUpdate()};if(this.chartMode==="bar"){this._pointTracker.dispose();const h=C((o=this._config)==null?void 0:o.entities);this._barTracker.bind(this.flow,h,e.barColor,s);return}this._barTracker.dispose(),this._pointTracker.bind(this.flow,r,e.maxPoints,s)}getGridOptions(){return{columns:12,rows:4,min_columns:6,min_rows:3}}renderChart2d(){if(!this._config)return m;const r=f(this._config);this._dataRevision;const e=this._config.name?String(this._config.name):"";return y`
      <ha-card>
        ${e?y`<h1 class="card-header">${e}</h1>`:m}
        <ha-chart-2d
          .mode=${this.chartMode}
          .points=${this._pointTracker.points}
          .bars=${this._barTracker.bars}
          .bgColor=${r.bgColor}
          .lineWidth=${r.lineWidth}
          .lineColor=${r.lineColor}
          .symbolSize=${r.symbolSize}
          .pointColor=${r.pointColor}
          .barColor=${r.barColor}
          .xLabel=${r.xLabel}
          .yLabel=${r.yLabel}
          .showAxes=${r.showAxes}
          .showGrid=${r.showGrid}
        ></ha-chart-2d>
      </ha-card>
    `}}j([l({attribute:!1})],D.prototype,"flow");export{D as H,U as c};
