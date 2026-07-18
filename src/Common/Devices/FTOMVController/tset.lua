--定义按钮
local log=Controls["Log"]
local ID= Controls["ID"]
--根据客户端数量添加按钮即可，代码不需改动
local play = Controls["Play"]
--根据客户端数量添加按钮即可，代码不需改动,数量与ID对应
local stop = Controls["Stop"]
--根据客户端数量添加按钮即可，代码不需改动,数量与ID对应
local port = Controls["Port"]
local ID= Controls["ID"]
--根据客户端数量添加按钮即可，代码不需改动,数量与ID对应
local Connect = Controls["Connect"]
--根据客户端数量添加按钮即可，代码不需改动,数量与ID对应 
local playing = Controls["Playing"]
--根据客户端数量添加按钮即可，代码不需改动,数量与ID对应

list={}
--所有已连接的客户端


for i, j in ipairs(Connect) do
  j.Value = 2
end

azhzplay = "\xa0\x00\x1f\x31\x7b\x22\x74\x70\x22\x3a\x22\x31\x22\x2c\x22\x63\x6f\x6d\x22\x3a\x22\x31\x30\x22\x2c\x22\x70\x6d\x22\x3a\x22\x31\x30\x22\x7d"
azhzstop = "\xa0\x00\x1f\x31\x7b\x22\x74\x70\x22\x3a\x22\x31\x22\x2c\x22\x63\x6f\x6d\x22\x3a\x22\x31\x30\x22\x2c\x22\x70\x6d\x22\x3a\x22\x31\x37\x22\x7d"
woshou   = "\xa0\x00\x1c\x31\x7b\x22\x74\x70\x22\x3a\x22\x31\x22\x2c\x22\x63\x6f\x6d\x22\x3a\x22\x39\x22\x2c\x22\x70\x6d\x22\x3a\x22\x22\x7d"

function hex2str(hex)
  --将16进制串转换为字符串
	--判断输入类型
	if (type(hex)~="string") then
		return nil,"hex2str invalid input type"
	end
	--拼接字符串
	local index=1
	local ret=""
	for index=1,hex:len() do
		ret=ret..string.format("%02X",hex:sub(index):byte())
	end
	return ret
end
--print(tonumber(hex2str('\x11'),16))
function judge_data_type(sock, event)
--判断指令类型
  if event == TcpSocket.Events.Data then
    status=hex2str(sock:Read(sock.BufferLength))
      if string.sub(status,53,56)=='6E61' then 

            if string.sub(status,65,66)~='22' then
               log.String="ID为 "..tonumber(string.char(tonumber(string.sub(status,63,64),16),tonumber(string.sub(status,65,66),16))).."的盒子收到握手指令"
               print("ID为"..tonumber(string.char(tonumber(string.sub(status,63,64),16),tonumber(string.sub(status,65,66),16))).."的盒子收到握手指令")
               list[tonumber(string.char(tonumber(string.sub(status,63,64),16),tonumber(string.sub(status,65,66),16)))] = sock --收到握手指令，此客户端为有效客户端，将此客户端存入list中，索引值为对应ID

            elseif string.sub(status,65,66)=='22' then
                   log.String="ID为"..tonumber(string.char(tonumber(string.sub(status,63,64),16))).."的盒子收到握手指令"
                   print("ID为"..tonumber(string.char(tonumber(string.sub(status,63,64),16))).."的盒子收到握手指令")
                   list[tonumber(string.char(tonumber(string.sub(status,63,64),16)))] = sock --收到握手指令，此客户端为有效客户端，将此客户端存入list中，索引值为对应ID 
            end 

        for k,v in pairs(list) do
            if v == sock then
                for i,d in pairs(ID) do
                    if tonumber(d.String) == k then
                       Connect[i].Value=0
                    end
                timer1 = Timer.New()
                local function timerFunc(timer)
                    for k,v in pairs(ID) do
                         if Connect[k].Value == 0 and timer == timer1 then
                            list[tonumber(ID[k].String)]:Write(woshou)
                            --向指定客户端握手
                            --print( ID[k].String.."握手" )  
                          end
                    end
                  end
              timer1.EventHandler = timerFunc
              timer1:Start(4)
              --定时器，单位为秒
              end
          end
        end 
      elseif string.sub(status,23,24)=='32' and string.sub(status,67,68)=='30' then
        for k,v in pairs(list) do
         if v==sock then
          for i,d in pairs(ID) do
            if tonumber(d.String)==k then
              playing[i].Value=1
         --else playing[i].Value=0
              print("ID为 "..k.." 的盒子播放")
              log.String="ID为 "..k.." 的盒子播放"
            --对应Playing LED播放示
            end
          end
        end
      end
        
    elseif  string.sub(status,23,24)=='32' and string.sub(status,67,68)=='32' then
      for k,v in pairs(list) do
          if v==sock then
            for i,d in pairs(ID) do
              if tonumber(d.String)==k then 
                playing[i].Value=0
                print("ID为 "..k.." 的盒子停止")
                log.String="ID为 "..k.." 的盒子停止"
              --对应Playing LED停止显示
              end
            end
          end 
      end
end
    elseif event == TcpSocket.Events.Closed or
           event == TcpSocket.Events.Error or
           event == TcpSocket.Events.Timeout then
         --Socket为超时、错误或关闭时
        for k,v in pairs(list) do
        if v==sock then
          for i,d in pairs(ID) do
            if tonumber(d.String)==k then
              playing[i].Value=0
              Connect[i].Value=2
            --对应playing和Connect LED停止显示
            end
          end
        end    
      end
      
      RemoveSocketFromTable(sock)
    --从客户端列表中移除该客户端
   end
   
end

server = TcpSocketServer.New()

function RemoveSocketFromTable(sock)
  --socket移除函数
  for k,v in pairs(list) do
  --迭代器判定需移除客户端索引值
    if v == sock then 
     break
     --跳出循环
    end
  end
  --log.String=k..'已断开'
  --print('客户端',k,'已断开')
  table.remove(list,k)
  --客户端列表中移除已断开的客户端，remove在循环中执行会报错，故在循环外删除
end


 
server.EventHandler = function(SocketInstance) --the properties of this socket instance are those of the TcpSocket library
  SocketInstance.ReadTimeout = 10
  --print( "客户端连接：", SocketInstance )
  SocketInstance.EventHandler = judge_data_type
  --socketserver收到指令，将数据传入judge_data_type函数
end

server:Listen(9090) 
-- TCP Server开始监听端口
 

for k,v in pairs(play) do
  --Controls[‘play’]中的所有按钮分别绑定向指定客户端发送播放指令
  play[k].EventHandler = function()
  status,err=pcall(function()
      list[tonumber(ID[k].String)]:Write(azhzplay)
      return ID[k].String..'播放指令发送成功'
      end)
  print(err)
  log.String=err
  --返回错误信息
  end
end


for k,v in pairs(stop) do
--Controls[‘stop’]中的所有按钮分别绑定向指定客户端发送停止指令
  stop[k].EventHandler = function()
  status,err=pcall(function()
      list[tonumber(ID[k].String)]:Write(azhzstop)
      return ID[k].String..'停止指令发送成功'
      end)
  print(err)
  log.String=err
  --返回错误信息
  
  end
end

for k,v in pairs(ID) do
--Controls[‘stop’]中的所有按钮分别绑定向指定客户端发送停止指令
  ID[k].EventHandler = function()
  status,err=pcall(function()
      list[tonumber(ID[k].String)]:Write(azhzstop)
      return ID[k].String..'停止指令发送成功'
      end)
  print(err)
  log.String=err
  --返回错误信息
  
  end
end

for k,v in pairs(ID) do
--Controls[‘loop’]中的所有按钮分别绑定向指定客户端发送循环指令
  ID[k].EventHandler = function()
        playing[k].Value=0
        Connect[k].Value=2
        --ID更改后初始化对应指示灯
  end
end
