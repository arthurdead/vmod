//===	======= Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose: Script initially run after squirrel VM is initialized
//
//=============================================================================

//-----------------------------------------------------------------------------
// General
//-----------------------------------------------------------------------------

::realPrint <- ::print;
::print_indent <- 0;

::print = function( text )
{
	for ( local i = ::print_indent; i > 0; --i )
	{
		::realPrint( "  " );
	}
	::realPrint( text );
};

::printl <- function( text )
{
	return ::print( text + "\n" );
};

::Msg <- function( text )
{
	return ::print( text );
};

::Assert <- function( b, msg = null )
{
	if ( b )
		return;
		
	if ( msg != null )
	{
		throw "Assertion failed: " + msg;
	}
	else
	{
		throw "Assertion failed";
	}
};

//-----------------------------------------------------------------------------

::FindCircularReference <- function( target )
{
	local visits = {}
	local result = false;
	
	local function RecursiveSearch( current )
	{
		if ( current in visits )
		{
			return;
		}
		visits[current] <- true;
		
		foreach( key, val in current )
		{
			if ( val == target && !::IsWeakref( target, key ) )
			{
				::print( "    Circular reference to " + target.tostring() + " in key " + key.tostring() + " slot " + val.tostring() + " of object " + current.tostring() + "\n" );
				result = true;
			}
			else if ( typeof( val ) == "table" || typeof( val ) == "array" || typeof( val ) == "instance" )
			{
				if ( !::IsWeakref( target, key ) )
				{
					RecursiveSearch( val );
				}
			}
		}
	};
	
	if ( typeof( target ) == "table" || typeof( target ) == "array" || typeof( target ) == "instance" )
		RecursiveSearch( target );
		
	return result;
};

::FindCircularReferences <- function( resurrecteds )
{
	::printl( "Circular references:" );

	if ( resurrecteds == null )
	{
		::printl( "    None");
		return;
	}
	
	if ( typeof( resurrecteds ) != "array" )
	{
		throw "Bad input to FindCircularReference";
	}

	foreach( val in resurrecteds )
	{
		::FindCircularReference( val );
	}
	
	::print("Resurrected objects: ");
	::DumpObject( resurrecteds );
};

//-----------------------------------------------------------------------------

::ScriptDebugDumpKeys <- function( name, table = null )
{
	if ( table == null )
	{
		table = ::getroottable();
	}

	if ( name == "" )
	{
		::printl( table.tostring() + "\n{" );
	}
	else
	{
		::printl( "Find \"" + name + "\"\n{" );
	}
	
	local function PrintKey( keyPath, key, value )
	{
		::printl( "    " + keyPath + " = " + value ); 
	};
	
	::ScriptDebugIterateKeys( name, PrintKey, table );
	
	::printl( "}" );
};

//-----------------------------------------------------------------------------

::ScriptDebugIterateKeys <- function( name, callback, table = null )
{
	local visits = {};
	local pattern;
	
	local function MatchRegexp( keyPath )
	{
		return pattern.match( keyPath );
	}
	
	local function MatchSubstring( keyPath )
	{
		return keyPath.find( name ) != null;
	}

	local function MatchAll( keyPath )
	{
		return true;
	}
	
	local matchFunc;
	
	if ( table == null )
	{
		table = ::getroottable();
	}

	if ( name == "" )
	{
		matchFunc = MatchAll;
	}
	else if ( name[0] == '#' ) // exact
	{
		pattern = ::regexp( "^" + name + "$" );
		matchFunc = MatchRegexp;
	}
	else if ( name[0] == '@' ) // regexp
	{
		pattern = ::regexp( name.slice( 1 ) );
		matchFunc = MatchRegexp;
	}
	else // general
	{
		matchFunc = MatchSubstring;
	}
		
	::ScriptDebugIterateKeysRecursive( matchFunc, null, table, visits, callback );
};

//-----------------------------------------------------------------------------

::ScriptDebugIterateKeysRecursive <- function( matchFunc, path, current, visits, callback )
{
	if ( ! ( current in visits ) )
	{
		visits[current] <- true;
		
		foreach( key, value in current )
		{
			if ( typeof(key) == "string" )
			{
				local keyPath = ( path ) ? path + "." + key : key;
				if ( matchFunc(keyPath) )
				{
					callback( keyPath, key, value );
				}
				
				if ( typeof(value) == "table" )
				{
					::ScriptDebugIterateKeysRecursive( matchFunc, keyPath, value, visits, callback );
				}
			}
		}
	}
};

//-----------------------------------------------------------------------------
// Documentation table
//-----------------------------------------------------------------------------

if ( ::developer() > 0 )
{
	::Documentation <-
	{
		classes = {}
		functions = {}
		instances = {}
	};

	::RetrieveNativeSignature <- function( nativeFunction )
	{
		if ( nativeFunction in ::NativeFunctionSignatures )
		{
			return ::NativeFunctionSignatures[nativeFunction];
		}
		return "<unnamed>";
	};
	
	::RegisterFunctionDocumentation <- function( func, name, signature, description )
	{
		if ( description.len() > 0 )
		{
			local b = ( description[0] == '#' );
			if ( description[0] == '#' )
			{
				local colon = description.find( ":" );
				if ( colon == null )
				{
					colon = description.len();
				}
				local alias = description.slice( 1, colon );
				description = description.slice( colon + 1 );
				name = alias;
				signature = "#";
			}
		}
		::Documentation.functions[name] <- [ signature, description, GetFunctionSourceFile(func) ];
	};

	::Document <- function( symbolOrTable, itemIfSymbol = null, descriptionIfSymbol = null )
	{
		if ( typeof( symbolOrTable ) == "table" )
		{
			foreach( symbol, itemDescription in symbolOrTable )
			{
				::Assert( typeof(symbol) == "string" )
				
				::Document( symbol, itemDescription[0], itemDescription[1] );
			}
		}
		else
		{
			::printl( symbolOrTable + ":" + itemIfSymbol.tostring() + "/" + descriptionIfSymbol );
		}
	};
	
	::PrintHelp <- function( string = "*", exact = false )
	{
		local matches = [];
		
		if ( string == "*" || !exact )
		{
			foreach( name, documentation in ::Documentation.functions )
			{
				if ( string != "*" && name.tolower().find( string.tolower() ) == null )
				{
					continue;
				}
				
				matches.append( name ); 
			}
		} 
		else if ( exact )
		{
			if ( string in ::Documentation.functions )
				matches.append( string );
		}
		
		if ( matches.len() == 0 )
		{
			::printl( "Symbol " + string + " not found" );
			return;
		}
		
		matches.sort();
		
		foreach( name in matches )
		{
			local result = name;
			local documentation = ::Documentation.functions[name];
			
			::printl( "Function:    " + name );
			local signature;
			if ( documentation[0] != "#" )
			{
				signature = documentation[0];
			}
			else
			{
				signature = ::GetFunctionSignature( ::getroottable()[name], name );
			}
			
			::printl( "Signature:   " + signature );
			if ( documentation[1].len() )
				::printl( "Description: " + documentation[1] );
			if ( documentation[2] != null && documentation[2].len() )
				::printl( "Script: " + documentation[2] );
			::print( "\n" ); 
		}
	}
}
else
{
	::RetrieveNativeSignature <- function( nativeFunction ) { return "<unnamed>"; };
	::RegisterFunctionDocumentation <- function( func, name, signature, description, script = "" ) {};
	::Document <- function( symbolOrTable, itemIfSymbol = null, descriptionIfSymbol = null ) {};
	::PrintHelp <- function( string = "*", exact = false )
	{
		::printl( "You must have started the script VM in developer mode to use this. Start a map/the app with '-dev'" );
	};
}

//-----------------------------------------------------------------------------
// VSquirrel support functions
//-----------------------------------------------------------------------------

::VSquirrel_OnCreateScope <- function( name, outer )
{
	local result;
	if ( !(name in outer) )
	{
		outer[name] <- { __vname=name, __vrefs = 1 }
		result = outer[name];
		result.setdelegate( outer );
	}
	else
	{
		result = outer[name];
		result.__vrefs += 1;
	}
	return result;
};

::VSquirrel_OnReleaseScope <- function( scope )
{
	scope.__vrefs -= 1;
	if ( scope.__vrefs < 0 )
	{
		throw "Bad reference counting on scope " + scope.__vname;
	}
	else if ( scope.__vrefs == 0 )
	{
		delete scope.getdelegate()[scope.__vname];
		scope.__vname = null;
		scope.setdelegate( null );
	}
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
::CCallChainer <- class
{
	constructor( prefixString, scopeForThis = null )
	{
		this.prefix = prefixString;
		if ( scopeForThis != null )
			this.scope = scopeForThis;
		else
			this.scope = ::getroottable();
		this.chains = {};
		
		// Expose a bound global function to dispatch to this object
		this.scope[ "Dispatch" + prefixString ] <- this.Call.bindenv( this );
	}
	
	function PostScriptExecute() 
	{
		foreach( key, value in this.scope )
		{
			if ( typeof( value ) == "function" ) 
			{
				if ( key.find( this.prefix ) == 0 )
				{
					local key_noprefix = key.slice( this.prefix.len() );
					
					if ( !(key_noprefix in this.chains) )
					{
						//::print( "Creating new call chain " + key_noprefix + "\n");
						this.chains[key_noprefix] <- [];
					}
					
					local chain = this.chains[key_noprefix];
					
					if ( !chain.len() || chain.top() != value )
					{
						chain.push( value );
						//::print( "Added " + value + " to call chain " + key_noprefix + "\n" );
					}
				}
			}
		}
	}
	
	function Call( event, ... )
	{
		if ( event in this.chains )
		{
			local chain = this.chains[event];
			if ( chain.len() )
			{
				local i;
				local args = [];
				if ( vargv.len() > 0 )
				{
					args.push( this.scope );
					for ( i = 0; i < vargv.len(); i++ )
					{
						args.push( vargv[i] );
					}
				}
				for ( i = chain.len() - 1; i >= 0; i -= 1 )
				{
					local func = chain[i];
					local result;
					if ( !args.len() )
					{
						result = func();
					}
					else
					{
						result = func.acall( args ); 
					}
					if ( result != null && !result )
						return false;
				}
			}
		}
		
		return true;
	}
	
	scope = null;
	prefix = null;
	chains = null;
};


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
::CSimpleCallChainer <- class
{
	constructor( prefixString, scopeForThis = null, exactNameMatch = false )
	{
		this.prefix = prefixString;
		if ( scopeForThis != null )
			this.scope = scopeForThis;
		else
			this.scope = ::getroottable();
		this.chain = [];
		
		// Expose a bound global function to dispatch to this object
		this.scope[ "Dispatch" + prefixString ] <- this.Call.bindenv( this );
		
		this.exactMatch = exactNameMatch;
	}
	
	function PostScriptExecute() 
	{
		foreach( key, value in this.scope )
		{
			if ( typeof( value ) == "function" ) 
			{
				local foundMatch = false;
				if ( this.exactMatch )
				{
					foundMatch = ( this.prefix == key );
				}
				else
				{
					foundMatch = ( key.find( this.prefix ) == 0 );
				}
						
				if ( foundMatch )
				{
					if ( !this.exactMatch )
						local key_noprefix = key.slice( this.prefix.len() );
					
					if ( !(this.chain) )
					{
						//::print( "Creating new call simple chain\n");
						this.chain <- [];
					}
					
					if ( !this.chain.len() || this.chain != value )
					{
						this.chain.push( value );
						//::print( "Added " + value + " to call chain.\n" );
					}
				}
			}
		}
	}
	
	function Call( ... )
	{
		if ( this.chain.len() > 0 )
		{
			local i;
			local args = [];
			if ( vargv.len() > 0 )
			{
				args.push( this.scope );
				for ( i = 0; i < vargv.len(); i++ )
				{
					args.push( vargv[i] );
				}
			}
			for ( i = this.chain.len() - 1; i >= 0; i -= 1 )
			{
				local func = this.chain[i];
				local result;
				if ( !args.len() )
				{
					result = func.pcall( this.scope );
				}
				else
				{
					result = func.pacall( this.scope, args ); 
				}
				if ( result != null && !result )
					return false;
			}
		}
		
		return true;
	}
	
	exactMatch = false;
	scope = null;
	prefix = null;
	chain = null;
};

//-----------------------------------------------------------------------------
// Late binding: allows a table to refer to parts of itself, it's children,
// it's owner, and then have the references fixed up after it's fully parsed
//
// Usage:
//    lateBinder <- LateBinder();
//    lateBinder.Begin( this );
//    
//    Test1 <-
//    {   
// 	   Foo=1
//    }   
//    
//    Test2 <-
//    {   
// 	   FooFoo = "I'm foo foo"
// 	   BarBar="@Test1.Foo"
// 	   SubTable = { boo=[bah, "@Test2.FooFoo", "@Test1.Foo"], booboo2={one=bah, two="@Test2.FooFoo", three="@Test1.Foo"} }
// 	   booboo=[bah, "@Test2.FooFoo", "@Test1.Foo"]
// 	   booboo2={one=bah, two="@Test2.FooFoo", three="@Test1.Foo"}
// 	   bah=wha
//    }   
//    
//    lateBinder.End();
//    delete lateBinder;
//
// When End() is called, all of the unresolved symbols in the tables and arrays will be resolved,
// any left unresolved will become a string prepended with '~', which later code can deal with
//-----------------------------------------------------------------------------

::LateBinder <- class
{
	// public:
	function Begin( target, log = false )
	{
		this.m_log = log;
		
		this.HookRootMetamethod( "_get", function( key ) { return "^" + key; } );
		this.HookRootMetamethod( "_newslot", function( key, value ) { if ( typeof value == "table" ) { this.m_fixupSet.push( [ key, value ] ); this.rawset( key, value ); };  }.bindenv(this) );
		this.m_targetTable = target;
		
		this.Log( "Begin late bind on table " + this.m_targetTable );
	}
	
	function End()
	{
		this.UnhookRootMetamethod( "_get" );
		this.UnhookRootMetamethod( "_newslot" );

		this.Log( "End late bind on table " + this.m_targetTable );
		
		foreach( subTablePair in this.m_fixupSet )
		{
			this.EstablishDelegation( this.m_targetTable, subTablePair[1] );
		}

		this.Log( "Begin resolution... " )
		this.m_logIndent++;
		
		local found = true;
		
		while ( found )
		{
			foreach( subTablePair in this.m_fixupSet )
			{
				this.Log( subTablePair[0] + " = " );
				this.Log( "{" );
				if ( !this.Resolve( subTablePair[1], subTablePair[1], false ) )
				{
					found = false;
				}
				this.Log( "}" );
			}
		}
			
		this.m_logIndent--;
		
		foreach( subTablePair in this.m_fixupSet )
		{
			this.RemoveDelegation( subTablePair[1] );
		}
		
		this.Log( "...end resolution" );
	}
		
	// private:
	function HookRootMetamethod( name, value )
	{
		local saved = null;
		local roottable = ::getroottable();
		if ( name in roottable )
		{
			saved = roottable[name];
		}
		roottable[name] <- value;
		roottable["__saved" + name] <- saved;
	}

	function UnhookRootMetamethod( name )
	{
		local saveSlot = "__saved" + name;
		local roottable = ::getroottable();
		local saved = roottable[saveSlot];
		if ( saved != null )
		{
			roottable[name] = saved;
		}
		else
		{
			delete roottable[name];
		}
		delete roottable[saveSlot];
	}

	function EstablishDelegation( parentTable, childTable )
	{
		childTable.setdelegate( parentTable );
		
		foreach( key, value in childTable )
		{
			local type = typeof value;
			if ( type == "table" )
			{
				this.EstablishDelegation( childTable, value );
			}
		}
	}
	
	function RemoveDelegation( childTable )
	{
		childTable.setdelegate( null );
		
		foreach( key, value in childTable )
		{
			local type = typeof value;
			if ( type == "table" )
			{
				this.RemoveDelegation( value );
			}
		}
	}

	function Resolve( lookupTable, subTableOrArray, throwException = false )
	{
		this.m_logIndent++;
		local found = false;
	
		foreach( key, value in subTableOrArray )
		{
			local type = typeof value;
			if ( type == "string" )
			{
				if ( value.len() )
				{
					local unresolvedId = null;
					local controlChar = value[0];
					if ( controlChar == '^' )
					{
						found = true;
						local value_clean = value.slice( 1 );
						if ( value_clean in lookupTable )
						{
							subTableOrArray[key] = lookupTable[value_clean];
							this.Log( key + " = " + lookupTable[value_clean] + " <-- " + value_clean );
						}
						else
						{
							subTableOrArray[key] = "~" + value_clean;
							unresolvedId = value_clean;
							this.Log( key + " = \"" + "~" + value_clean + "\" (unresolved)" );
						}
					}
					else if ( controlChar == '@' )
					{
						found = true;
						local identifiers = [];
						local iLast = 1;
						local iNext;
						while ( true )
						{
							iNext = value.find( ".", iLast );
							if(iNext == null) break;

							identifiers.push( value.slice( iLast, iNext ) );
							iLast = iNext + 1;
						}
						identifiers.push( value.slice( iLast ) );
						
						local depthSuccess = 0;
						local result = lookupTable;
						foreach( identifier in identifiers )
						{
							if ( identifier in result )
							{
								depthSuccess++;
								result = result[identifier];
							}
							else
							{
								break;
							}
						}
						if ( depthSuccess == identifiers.len() )
						{
							subTableOrArray[key] = result;
							this.Log( key + " = " + result + " <-- " + value );
						}
						else
						{
							subTableOrArray[key] = "~" + value.slice( 1 );
							unresolvedId = value;
							this.Log( key + " = \"" + "~" + value + "\" (unresolved)" );
						}
					}
					
					if ( unresolvedId != null )
					{
						if ( throwException )
						{
							//TODO!!!! where did bind come from?
							local bind = null;

							local exception = "Unresolved symbol: " + bind + " in ";
							foreach ( entry in this.m_bindNamesStack )
							{
								exception += entry;
								exception += "."
							}
							exception += unresolvedId;
							
							throw exception; 
						}
					}
				}
			}
		}

		foreach( key, value in subTableOrArray )
		{
			local type = typeof value;
			local isTable = ( type == "table" );
			local isArray = ( type == "array" );
			if ( isTable || isArray )
			{
				this.Log( key + " =" );
				this.Log( isTable ? "{" : "[" );
				
				this.m_bindNamesStack.push( key );
				if ( this.Resolve( ( isTable ) ? value : lookupTable, value, throwException ) )
				{
					found = true;
				}
				this.m_bindNamesStack.pop();
				
				this.Log( isTable ? "}" : "]" );
			}
		}
		this.m_logIndent--;
		return found;
	}
	
	function Log( string )
	{
		if ( this.m_log )
		{
			for ( local i = 0; i < this.m_logIndent; i++ )
			{
				::print( "  " );
			}
			
			::printl( string );
		}
	}

	m_targetTable = null;
	m_fixupSet = [];
	m_bindNamesStack = [];
	m_log = false;
	m_logIndent = 0;
};

// support function to assemble help strings for script calls - call once all your stuff is in the VM
::_PublishedHelp <- {};
//::AddToScriptHelp <- function( scopeTable )
function AddToScriptHelp( scopeTable )
{
	foreach (idx, val in scopeTable )
	{
		if (typeof(val) == "function")
		{
			local helpstr = "scripthelp_" + idx;
			if ( ( helpstr in scopeTable ) && ( ! (helpstr in ::_PublishedHelp) ) )
			{
//				::RegisterFunctionDocumentation( val, idx, "#", scopeTable[helpstr] );
				::RegisterFunctionDocumentation( val, idx, ::GetFunctionSignature( val, idx ), scopeTable[helpstr] );
				::_PublishedHelp[helpstr] <- true;
				::printl("Registered " + helpstr + " for " + val.tostring);
			}
		}
	}
};