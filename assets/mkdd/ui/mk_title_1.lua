
cycleTime = 4
startTint = vec4.new( 0, 100 / 255.0, 255 / 255.0, 0 )
endTint = vec4.new( 0, 0, 128 / 255.0, 1 )

currentTime = 0
scale = 1

function StartButton_Update()    
    currentTime = currentTime + Time.dt
    --local t = math.max( 0, math.min( 1, currentTime / cycleTime ) )
    local t = math.abs( math.sin( currentTime / cycleTime * math.pi ) )
    
    e.tint = Vec4Lerp( startTint, endTint, t )
end

function Startbutton_MouseButtonDown()
    if endTint.x == 0 then
        endTint.x = 1
    else
        endTint.x = 0
    end
    print( "Startbutton_MouseButtonDown" )
end

function Startbutton_MouseButtonUp()
    print( "Startbutton_MouseButtonUp" )
end