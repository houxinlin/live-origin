import { useState } from 'react'
import './App.css'
import Countries from './page/Countries'

function App() {
  const [currentPage, setCurrentPage] = useState<'main' | 'countries'>('countries')

  return (
    <>
      <Countries></Countries>
    </>
  )
}

export default App
