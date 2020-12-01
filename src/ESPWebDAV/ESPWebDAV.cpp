// WebDAV server using ESP8266 and SD card filesystem
// Targeting Windows 7 Explorer WebDav

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#endif
#include <SPI.h>
#include <Hash.h>
#include <time.h>

#include "ESPWebDAV.h"


// define cal constants
const char *months[] PROGMEM = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *wdays[] PROGMEM = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


// ------------------------
bool ESPWebDAV::init(int serverPort, FS* fileSystem) {
// ------------------------
	m_fileSystem = fileSystem;

	// start the wifi server
	server = new WiFiServer(serverPort);
	server->begin();

	return true;
}

// ------------------------
bool ESPWebDAV::stop() {
// ------------------------
	// start the wifi server
	server->stop();
	
	return true;
}


// ------------------------
void ESPWebDAV::handleNotFound() {
// ------------------------
	String message = "Not found\n";
	message += "URI: ";
	message += uri;
	message += " Method: ";
	message += method;
	message += "\n";

	sendHeader(F("Allow"), F("OPTIONS,MKCOL,POST,PUT"));
	send("404 Not Found", "text/plain", message);
	debugPrintln(F("404 Not Found"));
}



// ------------------------
void ESPWebDAV::handleReject(String rejectMessage)	{
// ------------------------
	debugPrint(F("Rejecting request: ")); debugPrintln(rejectMessage);

	// handle options
	if(method .equals(F("OPTIONS")))
		return handleOptions(RESOURCE_NONE);
	
	// handle properties
	if(method .equals(F("PROPFIND")))	{
		sendHeader(F("Allow"), F("PROPFIND,OPTIONS,DELETE,COPY,MOVE"));
		setContentLength(CONTENT_LENGTH_UNKNOWN);
		send("207 Multi-Status", "application/xml;charset=utf-8", "");
		sendContent(F("<?xml version=\"1.0\" encoding=\"utf-8\"?><D:multistatus xmlns:D=\"DAV:\"><D:response><D:href>/</D:href><D:propstat><D:status>HTTP/1.1 200 OK</D:status><D:prop><D:getlastmodified>Fri, 30 Nov 1979 00:00:00 GMT</D:getlastmodified><D:getetag>\"3333333333333333333333333333333333333333\"</D:getetag><D:resourcetype><D:collection/></D:resourcetype></D:prop></D:propstat></D:response>"));
		
		if(depthHeader .equals(F("1")))	{
			sendContent(F("<D:response><D:href>/"));
			sendContent(rejectMessage);
			sendContent(F("</D:href><D:propstat><D:status>HTTP/1.1 200 OK</D:status><D:prop><D:getlastmodified>Fri, 01 Apr 2016 16:07:40 GMT</D:getlastmodified><D:getetag>\"2222222222222222222222222222222222222222\"</D:getetag><D:resourcetype/><D:getcontentlength>0</D:getcontentlength><D:getcontenttype>application/octet-stream</D:getcontenttype></D:prop></D:propstat></D:response>"));
		}
		
		sendContent(F("</D:multistatus>"));
		return;
	}
	else
		// if reached here, means its a 404
		handleNotFound();
}




// set http_proxy=http://localhost:36036
// curl -v -X PROPFIND -H "Depth: 1" http://Rigidbot/Old/PipeClip.gcode
// Test PUT a file: curl -v -T c.txt -H "Expect:" http://Rigidbot/c.txt
// C:\Users\gsbal>curl -v -X LOCK http://Rigidbot/EMA_CPP_TRCC_Tutorial/Consumer.cpp -d "<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:lockinfo xmlns:D=\"DAV:\"><D:lockscope><D:exclusive/></D:lockscope><D:locktype><D:write/></D:locktype><D:owner><D:href>CARBON2\gsbal</D:href></D:owner></D:lockinfo>"
// ------------------------
void ESPWebDAV::handleRequest(String blank)	{
// ------------------------
	ResourceType resource = RESOURCE_NONE;

//#if defined USE_SPIFFS || defined USE_LITTLEFS
	// Truncate filename to 31 chars
	if (uri.length() > 31)
	{
		if (uri.endsWith("/"))
		{
			uri = uri.substring(0, 30) + "/";
		}
		else
		{
			int extPos = uri.lastIndexOf(".");
			String ext = uri.substring(extPos);
			String filename = uri.substring(0, 31 - ext.length());	
			uri = filename + ext;
		}
	}
//#endif

	// does uri refer to a file or directory or a null?
	if (m_fileSystem->exists(uri.c_str())) {
		File tFile= m_fileSystem->open(uri.c_str(), "r");
		resource = tFile.isDirectory() ? RESOURCE_DIR : RESOURCE_FILE;
		tFile.close();
	}

	debugPrint(F("\r\nm: ")); debugPrint(method);
	debugPrint(F(" r: ")); debugPrint(resource);
	debugPrint(F(" u: ")); debugPrintln(uri);

	// add header that gets sent everytime
	sendHeader(F("DAV"), F("2"));

	// handle properties
	if(method .equals(F("PROPFIND")))
		return handleProp(resource);
	
	if(method .equals(F("GET")))
		return handleGet(resource, true);

	if(method .equals(F("HEAD")))
		return handleGet(resource, false);

	// handle options
	if(method .equals(F("OPTIONS")))
		return handleOptions(resource);

	// handle file create/uploads
	if(method .equals(F("PUT")))
		return handlePut(resource);
	
	// handle file locks
	if(method .equals(F("LOCK")))
		return handleLock(resource);
	
	if(method .equals(F("UNLOCK")))
		return handleUnlock(resource);
	
	if(method .equals(F("PROPPATCH")))
		return handlePropPatch(resource);
	
	// directory creation
	if(method .equals(F("MKCOL")))
		return handleDirectoryCreate(resource);

	// move a file or directory
	if(method .equals(F("MOVE")))
		return handleMove(resource);
	
	// delete a file or directory
	if(method .equals(F("DELETE")))
		return handleDelete(resource);

	// if reached here, means its a 404
	handleNotFound();
}



// ------------------------
void ESPWebDAV::handleOptions(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing OPTION"));
	sendHeader(F("Allow"), F("PROPFIND,GET,DELETE,PUT,COPY,MOVE"));
	send("200 OK", NULL, "");
}



// ------------------------
void ESPWebDAV::handleLock(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing LOCK"));
	
	// does URI refer to an existing resource
	if(resource == RESOURCE_NONE)
		return handleNotFound();
	
	sendHeader(F("Allow"), F("PROPPATCH,PROPFIND,OPTIONS,DELETE,UNLOCK,COPY,LOCK,MOVE,HEAD,POST,PUT,GET"));
	sendHeader(F("Lock-Token"), F("urn:uuid:26e57cb3-834d-191a-00de-000042bdecf9"));

	size_t contentLen = contentLengthHeader.toInt();
	uint8_t buf[1024];
	size_t numRead = readBytesWithTimeout(buf, sizeof(buf), contentLen);
	
	if(numRead == 0)
		return handleNotFound();

	buf[contentLen] = 0;
	String inXML = String((char*) buf);
	int startIdx = inXML .indexOf(F("<D:href>"));
	int endIdx = inXML .indexOf(F("</D:href>"));
	if(startIdx < 0 || endIdx < 0)
		return handleNotFound();
		
	String lockUser = inXML.substring(startIdx + 8, endIdx);
	String resp1 = F("<?xml version=\"1.0\" encoding=\"utf-8\"?><D:prop xmlns:D=\"DAV:\"><D:lockdiscovery><D:activelock><D:locktype><write/></D:locktype><D:lockscope><exclusive/></D:lockscope><D:locktoken><D:href>urn:uuid:26e57cb3-834d-191a-00de-000042bdecf9</D:href></D:locktoken><D:lockroot><D:href>");
	String resp2 = F("</D:href></D:lockroot><D:depth>infinity</D:depth><D:owner><a:href xmlns:a=\"DAV:\">");
	String resp3 = F("</a:href></D:owner><D:timeout>Second-3600</D:timeout></D:activelock></D:lockdiscovery></D:prop>");

	send("200 OK", "application/xml;charset=utf-8", resp1 + uri + resp2 + lockUser + resp3);
}



// ------------------------
void ESPWebDAV::handleUnlock(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing UNLOCK"));
	sendHeader(F("Allow"), F("PROPPATCH,PROPFIND,OPTIONS,DELETE,UNLOCK,COPY,LOCK,MOVE,HEAD,POST,PUT,GET"));
	sendHeader(F("Lock-Token"), F("urn:uuid:26e57cb3-834d-191a-00de-000042bdecf9"));
	send("204 No Content", NULL, "");
}



// ------------------------
void ESPWebDAV::handlePropPatch(ResourceType resource)	{
// ------------------------
	debugPrintln(F("PROPPATCH forwarding to PROPFIND"));
	handleProp(resource);
}



// ------------------------
void ESPWebDAV::handleProp(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing PROPFIND"));
	// check depth header
	DepthType depth = DEPTH_NONE;
	if(depthHeader .equals(F("1")))
		depth = DEPTH_CHILD;
	else if(depthHeader .equals(F("infinity")))
		depth = DEPTH_ALL;
	
	debugPrint(F("Depth: ")); debugPrintln(depth);

	// does URI refer to an existing resource
	if(resource == RESOURCE_NONE)
		return handleNotFound();

	if(resource == RESOURCE_FILE)
		sendHeader(F("Allow"), F("PROPFIND,OPTIONS,DELETE,COPY,MOVE,HEAD,POST,PUT,GET"));
	else
		sendHeader(F("Allow"), F("PROPFIND,OPTIONS,DELETE,COPY,MOVE"));

	setContentLength(CONTENT_LENGTH_UNKNOWN);
	send("207 Multi-Status", "application/xml;charset=utf-8", "");
	sendContent(F("<?xml version=\"1.0\" encoding=\"utf-8\"?>"));
	sendContent(F("<D:multistatus xmlns:D=\"DAV:\">"));

	// open this resource
	File baseFile = m_fileSystem->open(uri.c_str(), "r");
	sendPropResponse(false, &baseFile);

	if((resource == RESOURCE_DIR) && (depth == DEPTH_CHILD))	{
		// append children information to message
		File childFile;
		while(childFile = baseFile.openNextFile()) {
			yield();
			sendPropResponse(true, &childFile);
			childFile.close();
		}
	}

	baseFile.close();
	sendContent(F("</D:multistatus>"));
}



// ------------------------
void ESPWebDAV::sendPropResponse(boolean recursing, File *curFile)	{
// ------------------------
	char buf[255];

// String fullResPath = "http://" + hostHeader + uri;
	String fullResPath = uri;

	if(recursing) {
		if(fullResPath.endsWith("/"))
			fullResPath += String(curFile->name());
		else
			fullResPath += "/" + String(curFile->name());		
	}
	fullResPath.replace(" ", "%20");


	// get file modified time
	time_t t2t = curFile->getLastWrite();
	tm *gTm = gmtime(&t2t);

	// Tue, 13 Oct 2015 17:07:35 GMT
	sprintf(buf, "%s, %02d %s %04d %02d:%02d:%02d GMT", wdays[gTm->tm_wday], gTm->tm_mday, months[gTm->tm_mon], gTm->tm_year + 1900, gTm->tm_hour, gTm->tm_min, gTm->tm_sec);
	String fileTimeStamp = String(buf);


	// send the XML information about thyself to client
	sendContent(F("<D:response><D:href>"));
	// append full file path
	sendContent(fullResPath);
	sendContent(F("</D:href><D:propstat><D:status>HTTP/1.1 200 OK</D:status><D:prop><D:getlastmodified>"));
	// append modified date
	sendContent(fileTimeStamp);
	sendContent(F("</D:getlastmodified><D:getetag>"));
	// append unique tag generated from full path
	sendContent("\"" + sha1(fullResPath + fileTimeStamp) + "\"");
	sendContent(F("</D:getetag>"));

	if(curFile->isDirectory())
		sendContent(F("<D:resourcetype><D:collection/></D:resourcetype>"));
	else	{
		sendContent(F("<D:resourcetype/><D:getcontentlength>"));
		// append the file size
		sendContent(String(curFile->size()));
		sendContent(F("</D:getcontentlength><D:getcontenttype>"));
		// append correct file mime type
		sendContent(getMimeType(fullResPath));
		sendContent(F("</D:getcontenttype>"));
	}
	sendContent(F("</D:prop></D:propstat></D:response>"));
}




// ------------------------
void ESPWebDAV::handleGet(ResourceType resource, bool isGet)	{
// ------------------------
	debugPrintln(F("Processing GET"));

	// does URI refer to an existing file resource
	if(resource != RESOURCE_FILE)
		return handleNotFound();

	long tStart = millis();
	uint8_t buf[1460];
	File rFile = m_fileSystem->open(uri.c_str(), "r");

	sendHeader(F("Allow"), F("PROPFIND,OPTIONS,DELETE,COPY,MOVE,HEAD,POST,PUT,GET"));
	size_t fileSize = rFile.size();
	setContentLength(fileSize);
	String contentType = getMimeType(uri);
	if(uri.endsWith(".gz") && contentType != F("application/x-gzip") && contentType != F("application/octet-stream"))
		sendHeader(F("Content-Encoding"), F("gzip"));

	send("200 OK", contentType.c_str(), "");

	if(isGet)	{
		// disable Nagle if buffer size > TCP MTU of 1460
		//client.setNoDelay(1);

		// send the file
		while(rFile.available())	{
			// SD read speed ~ 17sec for 4.5MB file
			int numRead = rFile.read(buf, sizeof(buf));
			client.write(buf, numRead);
		}
	}

	rFile.close();
	debugPrint(F("File ")); debugPrint(fileSize); debugPrint(F(" bytes sent in: ")); debugPrint((millis() - tStart)/1000); debugPrintln(F(" sec"));
}




// ------------------------
void ESPWebDAV::handlePut(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing Put"));

	// does URI refer to a directory
	if(resource == RESOURCE_DIR)
		return handleNotFound();

	File nFile;
	sendHeader(F("Allow"), F("PROPFIND,OPTIONS,DELETE,COPY,MOVE,HEAD,POST,PUT,GET"));

	// if file does not exist, create it
	if(resource == RESOURCE_NONE)	{
		nFile = m_fileSystem->open(uri.c_str(), "w+");
		if(!nFile)
			return handleWriteError(F("Unable to create a new file"), &nFile);
	}

	// file is created/open for writing at this point
	debugPrint(uri); debugPrintln(F(" - ready for data"));
	// did server send any data in put
	size_t contentLen = contentLengthHeader.toInt();

	if(contentLen != 0)	{
		// buffer size is critical *don't change*
		const size_t WRITE_BLOCK_CONST = 512;
		uint8_t buf[WRITE_BLOCK_CONST];
		long tStart = millis();
		size_t numRemaining = contentLen;


		// read data from stream and write to the file
		while(numRemaining > 0)	{
			size_t numToRead = (numRemaining > WRITE_BLOCK_CONST) ? WRITE_BLOCK_CONST : numRemaining;
			size_t numRead = readBytesWithTimeout(buf, sizeof(buf), numToRead);
			if(numRead == 0)
				break;

			// store whole buffer into file regardless of numRead
			if (!nFile.write(buf, sizeof(buf)))
				return handleWriteError(F("Write data failed"), &nFile);

			// reduce the number outstanding
			numRemaining -= numRead;
		}

		// detect timeout condition
		if(numRemaining)
			return handleWriteError(F("Timed out waiting for data"), &nFile);

		// truncate the file to right length
		if(!nFile.truncate(contentLen))
			return handleWriteError(F("Unable to truncate the file"), &nFile);

		nFile.close();
		debugPrint(F("File ")); debugPrint(contentLen - numRemaining); debugPrint(F(" bytes stored in: ")); debugPrint((millis() - tStart)/1000); debugPrintln(F(" sec"));
	}

	if(resource == RESOURCE_NONE)
		send("201 Created", NULL, "");
	else
		send("200 OK", NULL, "");

	nFile.close();
}




// ------------------------
void ESPWebDAV::handleWriteError(String message, File *wFile)	{
// ------------------------
	// close this file
	wFile->close();
	// delete the file being written
	m_fileSystem->remove(uri.c_str());
	// send error
	send("500 Internal Server Error", "text/plain", message);
	debugPrintln(message);
}


// ------------------------
void ESPWebDAV::handleDirectoryCreate(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing MKCOL"));
	
	// does URI refer to anything
	if(resource != RESOURCE_NONE)
		return handleNotFound();

//#if defined USE_LITTLEFS	
	if(uri.endsWith("/")) {
		uri = uri.substring(0, uri.length()-1);
	}
//#endif

	// create directory
	if (!m_fileSystem->mkdir(uri.c_str())) {
		// send error
		send("500 Internal Server Error", "text/plain", "Unable to create directory");
		debugPrintln(F("Unable to create directory"));
		return;
	}

	debugPrint(uri);	debugPrintln(F(" directory created"));
	sendHeader(F("Allow"), F("OPTIONS,MKCOL,LOCK,POST,PUT"));
	send("201 Created", NULL, "");
}



// ------------------------
void ESPWebDAV::handleMove(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing MOVE"));
	
	// does URI refer to anything
	if(resource == RESOURCE_NONE)
		return handleNotFound();

	if(destinationHeader.length() == 0)
		return handleNotFound();
		
	String dest = urlDecode(urlToUri(destinationHeader));

//#if defined USE_LITTLEFS
	if(uri.endsWith("/")) {
		uri = uri.substring(0, uri.length()-1);
	}

	if(dest.endsWith("/")) {
		dest = dest.substring(0, dest.length()-1);
	}
//#endif

	debugPrint(F("Move destination: ")); debugPrintln(dest);

	// move file or directory
	if ( !m_fileSystem->rename(uri.c_str(), dest.c_str())	) {
		// send error
		send("500 Internal Server Error", "text/plain", "Unable to move");
		debugPrintln(F("Unable to move file/directory"));
		return;
	}

	debugPrintln(F("Move successful"));
	sendHeader(F("Allow"), F("OPTIONS,MKCOL,LOCK,POST,PUT"));
	send("201 Created", NULL, "");
}


int ESPWebDAV::deleteRecursive(String path) {
	static int iCount;
	File file = m_fileSystem->open(path, "r");
	bool isDir = file.isDirectory();
	file.close();

	// If it's a plain file, delete it
	if (!isDir) {
		m_fileSystem->remove(path);
		iCount++;

		return iCount;
	}

	// Otherwise delete its contents first
	Dir dir = m_fileSystem->openDir(path);

	while (dir.next()) {
		deleteRecursive(path + '/' + dir.fileName());
	}

	// Then delete the folder itself
	m_fileSystem->rmdir(path);
	iCount++;

	return iCount;
}

// ------------------------
void ESPWebDAV::handleDelete(ResourceType resource)	{
// ------------------------
	debugPrintln(F("Processing DELETE"));
	
	// does URI refer to anything
	if(resource == RESOURCE_NONE)
		return handleNotFound();

	bool retVal = deleteRecursive(uri.c_str());
		
	if(!retVal)	{
		// send error
		send("500 Internal Server Error", "text/plain", "Unable to delete");
		debugPrintln(F("Unable to delete file/directory"));
		return;
	}

	debugPrint(F("Delete successful: ")); debugPrintln(retVal);
	sendHeader(F("Allow"), F("OPTIONS,MKCOL,LOCK,POST,PUT"));
	send("200 OK", NULL, "");
}
