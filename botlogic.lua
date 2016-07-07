function fromirc (line)
   pongres, wasping = line:gsub("PING :", "PONG :")
   if wasping == 1 then
      print(line)
      print("It was a PING! I'm gonna PONG!")
      return pongres, true
   end
   return "", false
end

