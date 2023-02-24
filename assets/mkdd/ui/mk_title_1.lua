
cycleTime = 2
startTint = vec4.new( 0, 100 / 255.0, 255 / 255.0, 0 )
endTint = vec4.new( 0, 0, 128 / 255.0, 1 )

currentTime = 0
scale = 1

function StartButton_Update()
    if currentTime >= cycleTime then
        scale = -1;
    elseif currentTime <= 0 then
        scale = 1;
    end
    
    currentTime = currentTime + scale * Time.dt
    local t = math.max( 0, math.min( 1, currentTime / cycleTime ) )
    
    e.tint = Vec4Lerp( startTint, endTint, t )
end