

function Start()
    --CreateLayout( "mk_title_1" )
    files = GetFilesProjectSubdir( "assets/cache/fastfiles/", false )
    for i=1,#files do
        local f = GetRelativeFilename( files[i] )
        if not f:find( "_required_v" ) then
            print( f:sub( 1, f:find( "_v20" ) - 1 ) )
        end
    end
end