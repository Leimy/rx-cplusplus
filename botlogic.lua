parameters = {
   nick    = "luabot",
   channel = "#radioxenu"
}

function fromirc (line)
   pongres, wasping = line:gsub("PING :", "PONG :")
   if wasping == 1 then
      print(line)
      print("It was a PING! I'm gonna PONG!")
      return pongres, true
   end
   if line:match("Howdy") then
      return string.format("PRIVMSG %s :Howdy your DAMN self!", parameters["channel"]), true
   end
   return "", false
end

