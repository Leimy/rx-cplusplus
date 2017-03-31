parameters = {
   nick    = "luabot",
   autolast = false,
   autotweet = false,
   channel = "",
   reqs = {},
   reqnext = 1,
   reqsmax = 5
}
function genNext (i)
   local t = (i + 1) % (parameters.reqsmax + 1)
   if t == 0 then
      return 1
   end
   return t
end

function toChannel (message)
   return string.format("PRIVMSG %s :%s", parameters.channel, message)
end

function parseReq (line)
   return line:match("%?request: (.*)%?")
end

function tprint (tbl, indent)
  if not indent then indent = 0 end
  for k, v in pairs(tbl) do
    formatting = string.rep("  ", indent) .. k .. ": "
    if type(v) == "table" then
      print(formatting)
      tprint(v, indent+1)
    elseif type(v) == 'boolean' then
      print(formatting .. tostring(v))
    else
      print(formatting .. v)
    end
  end
end

function procReq (line)
   local req = parseReq(line)
   if req == nil then
      return toChannel("Sorry, I didn't get that. Try ?request: Artist - Title?"), true
   end
   parameters.reqs[parameters.reqnext] = req
   parameters.reqnext = genNext(parameters.reqnext)
   tprint(parameters.reqs, 1)

   return toChannel("Thanks, got your request for" .. " " .. req .. "!"), true
end

function showReqs (line)
   local outtable = {}
   tprint(parameters, 1)
   -- set i to a reasonable choice for oldest
   local i = parameters.reqnext
   -- find the oldest non-nil, or if we wrap around -> bail
   while parameters.reqs[i] == nil do
      i = genNext(i)
      if i == parameters.reqnext then
	 return nil, false
      end
   end
   repeat
      outtable[#outtable + 1] = toChannel(parameters.reqs[i])
      i = genNext(i)
   until i == parameters.reqnext
   tprint(outtable, 1)
   return outtable, true
end

function nukeReqs ()
   parameters.reqs = {}
   return toChannel("Got it, the list, it is no more!!!"), true
end

function toggleAutoLast ()
   parameters.autolast = not parameters.autolast
   if parameters.autolast == true then
      return toChannel("autolast is ON"), true
   else
      return toChannel("autolast is OFF"), true
   end
end

function toggleAutoTweet ()
  parameters.autotweet = not parameters.autotweet
  if parameters.autotweet == true then
     return toChannel("autotweet is ON"), true
  else
     return toChannel("autotweet is OFF"), true
  end
end

function getLast ()
   return toChannel(md.getMetaData()), true
end

function tweetLast ()
   local meta = md.getMetaData()
   os.execute("./tweetit '" .. meta .. "'")
   return toChannel("tweeted: " .. meta), true
end

-- Must process all metadata update actions here
function onUpdateMetadata (metadata)
   if parameters.autotweet == true then
     os.execute("./tweetit '" .. metadata .. "'") 
   end
   if parameters.autolast == true then
      return toChannel(metadata), true
   end
   return true, true
end

function fromirc (line)
   -- Handle PING
   pongres, wasping = line:gsub("PING :", "PONG :")
   if wasping == 1 then
      print(line)
      print("It was a PING! I'm gonna PONG!")
      return pongres, true
   end
   -- just a test
   if line:match("Howdy") then
      return toChannel("Howdy your DAMN self!"), true
   end
   -- request handling
   if line:match("?request") then
      return procReq(line)
   end
   -- show reqs to the channel.
   if line:match("?showreqs?") then
      return showReqs(line)
   end
   -- auto last song toggle
   if line:match("?autolast?") then
      return toggleAutoLast()
   end
   -- auto tweet song toggle
   if line:match("?autotweet?") then
      return toggleAutoTweet()
   end
   -- blow away the requesticles
   if line:match("?nukereqs?") then
      return nukeReqs()
   end
   if line:match("?lastsong?") then
      return getLast()
   end
   if line:match("?tweet?") then
      return tweetLast()
   end
   return nil, false
end
