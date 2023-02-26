
startGame = nil
records = nil
options = nil

function Start()
    root = GetLayoutRootElement( "mk_title_1" )
    startGame = GetChildElement( root:Handle(), 2 )
    records = GetChildElement( root:Handle(), 3 )
    options = GetChildElement( root:Handle(), 4 )
end

cycleTime = 4
startTint = vec4.new( 0, 100 / 255.0, 255 / 255.0, 0 )
endTint = vec4.new( 0, 0, 128 / 255.0, 1 )

currentTime = 0
scale = 1

function StartButton_Update()    
    currentTime = currentTime + Time.dt
    local t = math.abs( math.sin( currentTime / cycleTime * math.pi ) )
    
    e.tint = Vec4Lerp( startTint, endTint, t )
end

function Startbutton_MBD()
    e:SetVisible( false )
    startGame:SetVisible( true )
    records:SetVisible( true )
    options:SetVisible( true )
end

function StartGame_MBD()
    print( "StartGame_MBD" )
end

function Records_MBD()
    print( "Records_MBD" )
end

function Options_MBD()
    print( "Options_MBD" )
end