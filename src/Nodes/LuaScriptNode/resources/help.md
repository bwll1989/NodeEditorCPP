
``` lua
--Node作为节点在lua中的命名空间，对节点中的操作需要加上此命名空间

Node:getTableValue("In",0)
-- 获取输入接口0，的值，返回值为table类型
Node:getTableValue(0)
-- 获取输出接口0，的值，返回值为table类型
Node:setTableValue(0,Map)
-- 设置输出节点0的值为Map，map的类型为经过中间层转换的类
Node:inputCount()
-- 获取输入端口数量
Node:outputCount()
-- 获取输出接口数量
Node:inputIndex()
-- 获取当前输入接口的索引,同时有的话默认显示后面的一个
```

``` lua
local luaTable = {
    key1 = "ewq",
    key2 = "hello",
    key3 = 12321
}
-- 由于需要在C++和lua中进行数据转换，VariantMap作为转换器使用
loacl map=VariantMap.fromLuaTable(fruits)
Node:setTableValue(0,map)
-- 由于需要在经过包装的lua table可以直接输出到对应端口
local updatedTable = map:toLuaTable()
--通过VariantMap中toLuaTable方法可以将C++中的值转换到lua中
```
``` lua
local luaTable = {
    key1 = Node:outputCount(),
    key2 = Node:inputCount(),
    key3 = 12321,
    key4 = Node:getTableValue("In",0):toLuaTable()["default"]
}

local out= VariantMap.fromLuaTable(luaTable)
Node:setTableValue(0,out)

-- 将端口0的输入与lua表合并后从输出0输出
``` 
``` lua


local port = Node:inputIndex()

if port == 0 then
    local out = VariantMap.fromLuaTable({default = "fullscreen"})
    Node:setTableValue(0, out)
elseif port == 1 then
    local out = VariantMap.fromLuaTable({default = "toggle_pause"})
    Node:setTableValue(0, out)
elseif port == 2 then
    local out = VariantMap.fromLuaTable({default = "playlist_prev"})
    Node:setTableValue(0, out)
elseif port == 3 then
    local out = VariantMap.fromLuaTable({default = "playlist_next"})
    Node:setTableValue(0, out)
end

-- 通过lua脚本控制mpv播放器，四个输入端口分别对应全屏切换、播放暂停、下一个、上一个
```

注意：
``` lua
local luaTable = { "ewq","hello",12321}
-- lua中使用这种table时，在C++端会自动插上索引，变成以下样子：
{ "1":"ewq","2":"hello","3":12321}
-- 由于lua中索引从1开始，对应的索引也会从1开始
```