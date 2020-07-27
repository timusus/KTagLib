package com.simplecityapps.ktaglib.sample

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.DocumentsContract
import android.util.Log
import android.widget.Button
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.RecyclerView
import com.simplecityapps.ktaglib.AudioFile
import com.simplecityapps.ktaglib.KTagLib
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn

class MainActivity : AppCompatActivity() {

    private val exceptionHandler = CoroutineExceptionHandler { _, throwable ->
        Log.e("MainActivity", "Coroutine failed: ${throwable.localizedMessage}")
    }

    private val scope = CoroutineScope(Dispatchers.Main + exceptionHandler)

    private lateinit var documentAdapter: DocumentAdapter

    private val tagLib = KTagLib()


    // Lifecycle

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_main)

        documentAdapter = DocumentAdapter()

        val recyclerView = findViewById<RecyclerView>(R.id.recyclerView)
        recyclerView.adapter = documentAdapter
        recyclerView.addItemDecoration(SpacesItemDecoration(8, true))

        val chooseDirectoryButton: Button = findViewById(R.id.chooseDirectoryButton)
        chooseDirectoryButton.setOnClickListener {
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE)
            if (intent.resolveActivity(packageManager) != null) {
                startActivityForResult(intent, REQUEST_CODE_OPEN_DOCUMENT)
            } else {
                Toast.makeText(this, "Document provider not found", Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQUEST_CODE_OPEN_DOCUMENT && resultCode == Activity.RESULT_OK) {
            data?.let { intent ->
                intent.data?.let { uri ->
                    contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
                    scope.launch {
                        documentAdapter.clear()
                        val documents = parseUri(uri)
                        getTags(documents).collect { pair ->
                            documentAdapter.addItem(pair)
                        }
                    }
                } ?: Log.e(TAG, "Intent uri null")
            } ?: Log.e(TAG, "onActivityResult failed to handle result: Intent data null")
        }
    }

    override fun onDestroy() {
        scope.cancel()
        super.onDestroy()
    }


    // Private

    private suspend fun parseUri(uri: Uri): List<Document> {
        return withContext(Dispatchers.IO) {
            val childDocumentsUri = DocumentsContract.buildChildDocumentsUriUsingTree(uri, DocumentsContract.getTreeDocumentId(uri))
            traverse(uri, childDocumentsUri, mutableListOf())
        }
    }

    private fun traverse(treeUri: Uri, documentUri: Uri, documents: MutableList<Document> = mutableListOf()): List<Document> {
        contentResolver.query(
            documentUri,
            arrayOf(
                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                DocumentsContract.Document.COLUMN_MIME_TYPE
            ),
            null,
            null,
            null
        ).use { cursor ->
            cursor?.let {
                while (cursor.moveToNext()) {
                    val documentId = cursor.getString(cursor.getColumnIndex(DocumentsContract.Document.COLUMN_DOCUMENT_ID))
                    val displayName = cursor.getString(cursor.getColumnIndex(DocumentsContract.Document.COLUMN_DISPLAY_NAME))
                    val mimeType = cursor.getString(cursor.getColumnIndex(DocumentsContract.Document.COLUMN_MIME_TYPE))
                    val childDocumentUri = DocumentsContract.buildDocumentUriUsingTree(documentUri, documentId)
                    if (mimeType == DocumentsContract.Document.MIME_TYPE_DIR) {
                        traverse(treeUri, DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, documentId), documents)
                    } else {
                        if (mimeType.startsWith("audio") ||
                            arrayOf("mp3", "3gp", "mp4", "m4a", "m4b", "aac", "ts", "flac", "mid", "xmf", "mxmf", "midi", "rtttl", "rtx", "ota", "imy", "ogg", "mkv", "wav", "opus")
                                .contains(displayName.substringAfterLast('.'))
                        ) {
                            documents.add(Document(childDocumentUri, documentId, displayName, mimeType))
                        }
                    }
                }
            }
        } ?: Log.e(TAG, "Failed to iterate cursor (null)")

        return documents
    }

    private fun getTags(documents: List<Document>): Flow<Pair<AudioFile, Document>> {
        return flow {
            documents.forEach { document ->
                contentResolver.openFileDescriptor(document.uri, "r")?.use { pfd ->
                    tagLib.getAudioFile(pfd.detachFd(), document.uri.toString(), document.displayName.substringBeforeLast(".") ?: "Unknown")?.let { audioFile ->
                        emit(Pair(audioFile, document))
                    }
                }
            }
        }.flowOn(Dispatchers.IO)
    }


    // Static

    companion object {
        const val TAG = "MainActivity"
        const val REQUEST_CODE_OPEN_DOCUMENT = 100
    }
}